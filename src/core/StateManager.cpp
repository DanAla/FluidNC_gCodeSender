/**
 * core/StateManager.cpp
 * Implementation of thread-safe JSON-based state management
 */

#include "StateManager.h"
#include <fstream>
#include <chrono>
#include <iostream>
#include <sstream>

StateManager& StateManager::getInstance()
{
    static StateManager instance;
    return instance;
}

StateManager::StateManager() : m_stopAutosave(false)
{
    createConfigDirs();
    load();
    
    // Start autosave thread
    m_autosaveThread = std::thread(&StateManager::autosaveLoop, this);
}

StateManager::~StateManager()
{
    shutdown();
}

void StateManager::save()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    try {
        std::ofstream file(m_settingsFile);
        if (file.is_open()) {
            file << m_data.dump(2) << std::endl;
            file.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving settings: " << e.what() << std::endl;
    }
}

void StateManager::saveRecovery()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    try {
        std::ofstream file(m_recoveryFile);
        if (file.is_open()) {
            file << m_data.dump() << std::endl;  // No indentation for speed
            file.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving recovery: " << e.what() << std::endl;
    }
}

void StateManager::shutdown()
{
    // Prevent double shutdown
    static bool shutdownCalled = false;
    if (shutdownCalled) {
        return;
    }
    shutdownCalled = true;
    
    if (m_autosaveThread.joinable()) {
        m_stopAutosave = true;
        m_autosaveThread.join();
    }
    save();  // Final save
}

void StateManager::load()
{
    try {
        if (std::filesystem::exists(m_settingsFile)) {
            std::ifstream file(m_settingsFile);
            if (file.is_open()) {
                file >> m_data;
                file.close();
                return;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading settings: " << e.what() << std::endl;
    }
    
    // Initialize with empty JSON object if loading failed
    m_data = json::object();
    initializeDefaults();
}

void StateManager::initializeDefaults()
{
    // Set default configuration values
    if (!m_data.contains("machines")) {
        m_data["machines"] = json::array();
    }
    if (!m_data.contains("windowLayouts")) {
        m_data["windowLayouts"] = json::array();
    }
    if (!m_data.contains("jobProfiles")) {
        m_data["jobProfiles"] = json::array();
    }
    if (!m_data.contains("currentJobSettings")) {
        JobSettings defaultJob;
        m_data["currentJobSettings"] = jobSettingsToJson(defaultJob);
    }
    if (!m_data.contains("activeMachine")) {
        m_data["activeMachine"] = "";
    }
}

void StateManager::autosaveLoop()
{
    while (!m_stopAutosave) {
        // Wait for 5 seconds or until stop signal
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
            if (m_stopAutosave) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (!m_stopAutosave) {
            saveRecovery();
        }
    }
}

void StateManager::createConfigDirs()
{
    try {
        std::filesystem::create_directories(m_configDir);
    } catch (const std::exception& e) {
        std::cerr << "Error creating config directory: " << e.what() << std::endl;
    }
}

json* StateManager::getNestedValue(const std::string& key)
{
    auto keys = splitKey(key);
    json* current = &m_data;
    
    for (const auto& k : keys) {
        if (!current->is_object() || !current->contains(k)) {
            return nullptr;
        }
        current = &(*current)[k];
    }
    
    return current;
}

void StateManager::setNestedValue(const std::string& key, const json& value)
{
    auto keys = splitKey(key);
    json* current = &m_data;
    
    // Navigate to parent, creating objects as needed
    for (size_t i = 0; i < keys.size() - 1; ++i) {
        if (!current->is_object()) {
            *current = json::object();
        }
        current = &(*current)[keys[i]];
    }
    
    // Set final value
    if (!current->is_object()) {
        *current = json::object();
    }
    (*current)[keys.back()] = value;
}

std::vector<std::string> StateManager::splitKey(const std::string& key)
{
    std::vector<std::string> result;
    std::stringstream ss(key);
    std::string item;
    
    while (std::getline(ss, item, '/')) {
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    
    return result;
}

// Machine management methods
std::vector<MachineConfig> StateManager::getMachines() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    std::vector<MachineConfig> machines;
    if (m_data.contains("machines") && m_data["machines"].is_array()) {
        for (const auto& machineJson : m_data["machines"]) {
            machines.push_back(machineConfigFromJson(machineJson));
        }
    }
    return machines;
}

void StateManager::addMachine(const MachineConfig& machine)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (!m_data.contains("machines")) {
        m_data["machines"] = json::array();
    }
    
    // Check if machine already exists
    for (auto& machineJson : m_data["machines"]) {
        if (machineJson["id"] == machine.id) {
            // Update existing
            machineJson = machineConfigToJson(machine);
            return;
        }
    }
    
    // Add new machine
    m_data["machines"].push_back(machineConfigToJson(machine));
}

void StateManager::updateMachine(const std::string& id, const MachineConfig& machine)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (m_data.contains("machines") && m_data["machines"].is_array()) {
        for (auto& machineJson : m_data["machines"]) {
            if (machineJson["id"] == id) {
                machineJson = machineConfigToJson(machine);
                return;
            }
        }
    }
}

void StateManager::removeMachine(const std::string& id)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (m_data.contains("machines") && m_data["machines"].is_array()) {
        auto& machines = m_data["machines"];
        machines.erase(std::remove_if(machines.begin(), machines.end(),
            [&id](const json& machineJson) {
                return machineJson.contains("id") && machineJson["id"] == id;
            }), machines.end());
    }
}

MachineConfig StateManager::getMachine(const std::string& id) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (m_data.contains("machines") && m_data["machines"].is_array()) {
        for (const auto& machineJson : m_data["machines"]) {
            if (machineJson.contains("id") && machineJson["id"] == id) {
                return machineConfigFromJson(machineJson);
            }
        }
    }
    
    return MachineConfig(); // Return empty config if not found
}

void StateManager::setActiveMachine(const std::string& id)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_data["activeMachine"] = id;
}

std::string StateManager::getActiveMachineId() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_data.contains("activeMachine")) {
        return m_data["activeMachine"].get<std::string>();
    }
    return "";
}

// Window layout management
std::vector<WindowLayout> StateManager::getWindowLayouts() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    std::vector<WindowLayout> layouts;
    if (m_data.contains("windowLayouts") && m_data["windowLayouts"].is_array()) {
        for (const auto& layoutJson : m_data["windowLayouts"]) {
            layouts.push_back(windowLayoutFromJson(layoutJson));
        }
    }
    return layouts;
}

void StateManager::saveWindowLayout(const WindowLayout& layout)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (!m_data.contains("windowLayouts")) {
        m_data["windowLayouts"] = json::array();
    }
    
    // Check if layout already exists
    bool updated = false;
    for (auto& layoutJson : m_data["windowLayouts"]) {
        if (layoutJson["windowId"] == layout.windowId) {
            // Update existing
            layoutJson = windowLayoutToJson(layout);
            updated = true;
            break;
        }
    }
    
    if (!updated) {
        // Add new layout
        m_data["windowLayouts"].push_back(windowLayoutToJson(layout));
    }
    
    // Save immediately to disk to ensure window state is preserved
    try {
        std::ofstream file(m_settingsFile);
        if (file.is_open()) {
            file << m_data.dump(2) << std::endl;
            file.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving window layout: " << e.what() << std::endl;
    }
}

WindowLayout StateManager::getWindowLayout(const std::string& windowId) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (m_data.contains("windowLayouts") && m_data["windowLayouts"].is_array()) {
        for (const auto& layoutJson : m_data["windowLayouts"]) {
            if (layoutJson.contains("windowId") && layoutJson["windowId"] == windowId) {
                return windowLayoutFromJson(layoutJson);
            }
        }
    }
    
    // Return default layout
    WindowLayout layout;
    layout.windowId = windowId;
    return layout;
}

void StateManager::resetWindowLayouts()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_data["windowLayouts"] = json::array();
}

// Job settings management
JobSettings StateManager::getCurrentJobSettings() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (m_data.contains("currentJobSettings")) {
        return jobSettingsFromJson(m_data["currentJobSettings"]);
    }
    
    return JobSettings(); // Return default settings
}

void StateManager::setCurrentJobSettings(const JobSettings& settings)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_data["currentJobSettings"] = jobSettingsToJson(settings);
}

std::vector<JobSettings> StateManager::getSavedJobProfiles() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    std::vector<JobSettings> profiles;
    if (m_data.contains("jobProfiles") && m_data["jobProfiles"].is_array()) {
        for (const auto& profileJson : m_data["jobProfiles"]) {
            profiles.push_back(jobSettingsFromJson(profileJson));
        }
    }
    return profiles;
}

void StateManager::saveJobProfile(const JobSettings& settings)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (!m_data.contains("jobProfiles")) {
        m_data["jobProfiles"] = json::array();
    }
    
    // Check if profile already exists
    for (auto& profileJson : m_data["jobProfiles"]) {
        if (profileJson["name"] == settings.name) {
            // Update existing
            profileJson = jobSettingsToJson(settings);
            return;
        }
    }
    
    // Add new profile
    m_data["jobProfiles"].push_back(jobSettingsToJson(settings));
}

void StateManager::deleteJobProfile(const std::string& name)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (m_data.contains("jobProfiles") && m_data["jobProfiles"].is_array()) {
        auto& profiles = m_data["jobProfiles"];
        profiles.erase(std::remove_if(profiles.begin(), profiles.end(),
            [&name](const json& profileJson) {
                return profileJson.contains("name") && profileJson["name"] == name;
            }), profiles.end());
    }
}

// Configuration file paths
std::string StateManager::getSettingsFilePath() const
{
    return m_settingsFile.string();
}

std::string StateManager::getRecoveryFilePath() const
{
    return m_recoveryFile.string();
}

// JSON conversion helper implementations
json StateManager::machineConfigToJson(const MachineConfig& config) const
{
    json j;
    j["name"] = config.name;
    j["id"] = config.id;
    j["type"] = connectionTypeToString(config.type);
    j["host"] = config.host;
    j["port"] = config.port;
    j["device"] = config.device;
    j["baudRate"] = config.baudRate;
    j["autoConnect"] = config.autoConnect;
    j["machineSettings"] = config.machineSettings;
    return j;
}

MachineConfig StateManager::machineConfigFromJson(const json& j) const
{
    MachineConfig config;
    if (j.contains("name")) config.name = j["name"].get<std::string>();
    if (j.contains("id")) config.id = j["id"].get<std::string>();
    if (j.contains("type")) config.type = connectionTypeFromString(j["type"].get<std::string>());
    if (j.contains("host")) config.host = j["host"].get<std::string>();
    if (j.contains("port")) config.port = j["port"].get<int>();
    if (j.contains("device")) config.device = j["device"].get<std::string>();
    if (j.contains("baudRate")) config.baudRate = j["baudRate"].get<int>();
    if (j.contains("autoConnect")) config.autoConnect = j["autoConnect"].get<bool>();
    if (j.contains("machineSettings")) config.machineSettings = j["machineSettings"];
    return config;
}

json StateManager::windowLayoutToJson(const WindowLayout& layout) const
{
    json j;
    j["windowId"] = layout.windowId;
    j["x"] = layout.x;
    j["y"] = layout.y;
    j["width"] = layout.width;
    j["height"] = layout.height;
    j["visible"] = layout.visible;
    j["docked"] = layout.docked;
    j["maximized"] = layout.maximized;
    j["dockingSide"] = layout.dockingSide;
    return j;
}

WindowLayout StateManager::windowLayoutFromJson(const json& j) const
{
    WindowLayout layout;
    if (j.contains("windowId")) layout.windowId = j["windowId"].get<std::string>();
    if (j.contains("x")) layout.x = j["x"].get<int>();
    if (j.contains("y")) layout.y = j["y"].get<int>();
    if (j.contains("width")) layout.width = j["width"].get<int>();
    if (j.contains("height")) layout.height = j["height"].get<int>();
    if (j.contains("visible")) layout.visible = j["visible"].get<bool>();
    if (j.contains("docked")) layout.docked = j["docked"].get<bool>();
    if (j.contains("maximized")) layout.maximized = j["maximized"].get<bool>();
    if (j.contains("dockingSide")) layout.dockingSide = j["dockingSide"].get<std::string>();
    return layout;
}

json StateManager::jobSettingsToJson(const JobSettings& settings) const
{
    json j;
    j["name"] = settings.name;
    j["feedRate"] = settings.feedRate;
    j["spindleSpeed"] = settings.spindleSpeed;
    j["safeZ"] = settings.safeZ;
    j["workZ"] = settings.workZ;
    j["depthPerPass"] = settings.depthPerPass;
    j["material"] = settings.material;
    j["toolType"] = settings.toolType;
    j["toolDiameter"] = settings.toolDiameter;
    return j;
}

JobSettings StateManager::jobSettingsFromJson(const json& j) const
{
    JobSettings settings;
    if (j.contains("name")) settings.name = j["name"].get<std::string>();
    if (j.contains("feedRate")) settings.feedRate = j["feedRate"].get<float>();
    if (j.contains("spindleSpeed")) settings.spindleSpeed = j["spindleSpeed"].get<float>();
    if (j.contains("safeZ")) settings.safeZ = j["safeZ"].get<float>();
    if (j.contains("workZ")) settings.workZ = j["workZ"].get<float>();
    if (j.contains("depthPerPass")) settings.depthPerPass = j["depthPerPass"].get<float>();
    if (j.contains("material")) settings.material = j["material"].get<std::string>();
    if (j.contains("toolType")) settings.toolType = j["toolType"].get<std::string>();
    if (j.contains("toolDiameter")) settings.toolDiameter = j["toolDiameter"].get<float>();
    return settings;
}

// Utility functions for connection type conversion
std::string connectionTypeToString(ConnectionType type)
{
    switch (type) {
        case ConnectionType::Telnet: return "Telnet";
        case ConnectionType::USB: return "USB";
        case ConnectionType::UART: return "UART";
        default: return "Unknown";
    }
}

ConnectionType connectionTypeFromString(const std::string& str)
{
    if (str == "Telnet") return ConnectionType::Telnet;
    if (str == "USB") return ConnectionType::USB;
    if (str == "UART") return ConnectionType::UART;
    return ConnectionType::Telnet; // Default
}
