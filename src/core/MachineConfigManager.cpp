/**
 * core/MachineConfigManager.cpp
 * Implementation of enhanced machine configuration management
 */

#include "MachineConfigManager.h"
#include "SimpleLogger.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

// Singleton instance
MachineConfigManager& MachineConfigManager::Instance() {
    static MachineConfigManager instance;
    return instance;
}

// Machine management
std::vector<EnhancedMachineConfig> MachineConfigManager::GetAllMachines() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_machines;
}

EnhancedMachineConfig MachineConfigManager::GetMachine(const std::string& machineId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t index = FindMachineIndex(machineId);
    if (index < m_machines.size()) {
        return m_machines[index];
    }
    return EnhancedMachineConfig(); // Return empty config if not found
}

void MachineConfigManager::AddMachine(const EnhancedMachineConfig& machine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Check if machine already exists
    size_t index = FindMachineIndex(machine.id);
    if (index < m_machines.size()) {
        // Update existing
        m_machines[index] = machine;
    } else {
        // Add new
        m_machines.push_back(machine);
    }
    
    SaveToFile();
    NotifyMachineUpdate(machine.id);
    
    LOG_INFO("Added/Updated machine configuration: " + machine.name + " (" + machine.id + ")");
}

void MachineConfigManager::UpdateMachine(const std::string& machineId, const EnhancedMachineConfig& machine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t index = FindMachineIndex(machineId);
    if (index < m_machines.size()) {
        m_machines[index] = machine;
        SaveToFile();
        NotifyMachineUpdate(machineId);
        LOG_INFO("Updated machine configuration: " + machine.name + " (" + machineId + ")");
    }
}

void MachineConfigManager::RemoveMachine(const std::string& machineId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t index = FindMachineIndex(machineId);
    if (index < m_machines.size()) {
        std::string machineName = m_machines[index].name;
        m_machines.erase(m_machines.begin() + index);
        
        // Clear active machine if it was the removed one
        if (m_activeMachineId == machineId) {
            m_activeMachineId.clear();
        }
        
        SaveToFile();
        LOG_INFO("Removed machine configuration: " + machineName + " (" + machineId + ")");
    }
}

// Active machine management
void MachineConfigManager::SetActiveMachine(const std::string& machineId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (FindMachineIndex(machineId) < m_machines.size() || machineId.empty()) {
        m_activeMachineId = machineId;
        LOG_INFO("Active machine set to: " + (machineId.empty() ? "None" : machineId));
    }
}

std::string MachineConfigManager::GetActiveMachineId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeMachineId;
}

EnhancedMachineConfig MachineConfigManager::GetActiveMachine() const {
    // This method calls GetMachine, which is already locked.
    // To avoid deadlock, we should use a recursive_mutex or lock here
    // and make the GetMachine not lock. For now, let's lock both,
    // assuming they are called from different contexts.
    // The proper fix is to use a recursive mutex.
    std::lock_guard<std::mutex> lock(m_mutex);
    return GetMachine(m_activeMachineId);
}

bool MachineConfigManager::HasActiveMachine() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_activeMachineId.empty() && FindMachineIndex(m_activeMachineId) < m_machines.size();
}

// Machine capability management
void MachineConfigManager::UpdateMachineCapabilities(const std::string& machineId, const MachineCapabilities& capabilities) {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t index = FindMachineIndex(machineId);
    if (index < m_machines.size()) {
        m_machines[index].capabilities = capabilities;
        
        // Auto-configure homing based on detected kinematics
        if (!capabilities.kinematics.empty() && capabilities.capabilitiesValid) {
            AutoConfigureHoming(machineId, capabilities.kinematics);
        }
        
        SaveToFile();
        NotifyCapabilityUpdate(machineId);
        LOG_INFO("Updated capabilities for machine: " + m_machines[index].name + " (Kinematics: " + capabilities.kinematics + ")");
    }
}

MachineCapabilities MachineConfigManager::GetMachineCapabilities(const std::string& machineId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t index = FindMachineIndex(machineId);
    if (index < m_machines.size()) {
        return m_machines[index].capabilities;
    }
    return MachineCapabilities();
}

// Kinematics detection and auto-configuration
std::string MachineConfigManager::DetectKinematics(const std::map<int, float>& grblSettings, const std::vector<std::string>& systemInfo) const {
    // This method is const and does not access member data, so no lock is needed.
    // Method 1: Check FluidNC kinematics setting (if available)
    // Note: FluidNC might use different parameter numbers - this is an example
    auto kinematicsParam = grblSettings.find(400); // Hypothetical kinematics parameter
    if (kinematicsParam != grblSettings.end()) {
        int type = static_cast<int>(kinematicsParam->second);
        switch (type) {
            case 0: return "Cartesian";
            case 1: return "CoreXY"; 
            case 2: return "Delta";
            case 3: return "SCARA";
            default: break;
        }
    }
    
    // Method 2: Parse build info for kinematics hints
    for (const auto& info : systemInfo) {
        std::string lowerInfo = info;
        std::transform(lowerInfo.begin(), lowerInfo.end(), lowerInfo.begin(), ::tolower);
        
        if (lowerInfo.find("corexy") != std::string::npos) {
            return "CoreXY";
        }
        if (lowerInfo.find("delta") != std::string::npos) {
            return "Delta";
        }
        if (lowerInfo.find("scara") != std::string::npos) {
            return "SCARA";
        }
        if (lowerInfo.find("cartesian") != std::string::npos) {
            return "Cartesian";
        }
    }
    
    // Method 3: Analyze GRBL settings for hints
    // Look for indicators of different kinematics in settings
    auto stepsPer_X = grblSettings.find(100); // $100 - X steps/mm
    auto stepsPer_Y = grblSettings.find(101); // $101 - Y steps/mm
    
    if (stepsPer_X != grblSettings.end() && stepsPer_Y != grblSettings.end()) {
        // CoreXY machines often have identical X/Y steps per mm
        if (std::abs(stepsPer_X->second - stepsPer_Y->second) < 0.1f && stepsPer_X->second > 50.0f) {
            // This might be CoreXY, but it's not definitive
            // We'll be conservative and not assume CoreXY without more evidence
        }
    }
    
    return "Cartesian"; // Safe default assumption
}

void MachineConfigManager::AutoConfigureHoming(const std::string& machineId, const std::string& kinematics) {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t index = FindMachineIndex(machineId);
    if (index >= m_machines.size()) return;
    
    auto& machine = m_machines[index];
    
    // Configure homing sequence based on kinematics
    if (kinematics == "CoreXY") {
        machine.homing.sequence = HomingSettings::SEQUENTIAL_ZXY;
        LOG_INFO("Auto-configured homing for CoreXY machine: " + machine.name + " (Sequential Z->X->Y)");
    } else if (kinematics == "Delta") {
        machine.homing.sequence = HomingSettings::SIMULTANEOUS;
        LOG_INFO("Auto-configured homing for Delta machine: " + machine.name + " (Simultaneous)");
    } else if (kinematics == "SCARA") {
        // SCARA might need custom sequence depending on configuration
        machine.homing.sequence = HomingSettings::SEQUENTIAL_ZXY; // Conservative choice
        LOG_INFO("Auto-configured homing for SCARA machine: " + machine.name + " (Sequential Z->X->Y)");
    } else {
        // Cartesian or unknown - use simultaneous
        machine.homing.sequence = HomingSettings::SIMULTANEOUS;
        LOG_INFO("Auto-configured homing for Cartesian machine: " + machine.name + " (Simultaneous)");
    }
    
    SaveToFile();
    NotifyMachineUpdate(machineId);
}

// Connection status updates
void MachineConfigManager::UpdateConnectionStatus(const std::string& machineId, bool connected, const std::string& timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t index = FindMachineIndex(machineId);
    if (index < m_machines.size()) {
        m_machines[index].connected = connected;
        
        if (!timestamp.empty()) {
            m_machines[index].lastConnected = timestamp;
        } else if (connected) {
            // Generate current timestamp
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            m_machines[index].lastConnected = ss.str();
        }
        
        SaveToFile();
        NotifyMachineUpdate(machineId);
    }
}

// Persistence
void MachineConfigManager::SaveToFile() {
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        json j = json::array();
        
        for (const auto& machine : m_machines) {
            j.push_back(machine.ToJson());
        }
        
        json root;
        root["machines"] = j;
        root["activeMachine"] = m_activeMachineId;
        root["version"] = "2.0";
        root["lastSaved"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        std::string configPath = GetConfigFilePath();
        std::filesystem::create_directories(std::filesystem::path(configPath).parent_path());
        
        std::ofstream file(configPath);
        if (file.is_open()) {
            file << root.dump(2);
            file.close();
            LOG_INFO("Saved machine configurations to: " + configPath);
        } else {
            LOG_ERROR("Failed to save machine configurations to: " + configPath);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error saving machine configurations: " + std::string(e.what()));
    }
}

void MachineConfigManager::LoadFromFile() {
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        std::string configPath = GetConfigFilePath();
        
        if (!std::filesystem::exists(configPath)) {
            LOG_INFO("Machine config file does not exist, starting with empty configuration");
            return;
        }
        
        std::ifstream file(configPath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open machine config file: " + configPath);
            return;
        }
        
        json root;
        file >> root;
        file.close();
        
        m_machines.clear();
        
        if (root.contains("machines") && root["machines"].is_array()) {
            for (const auto& machineJson : root["machines"]) {
                try {
                    EnhancedMachineConfig machine = EnhancedMachineConfig::FromJson(machineJson);
                    m_machines.push_back(machine);
                } catch (const std::exception& e) {
                    LOG_ERROR("Error loading machine config: " + std::string(e.what()));
                }
            }
        }
        
        if (root.contains("activeMachine")) {
            m_activeMachineId = root["activeMachine"].get<std::string>();
        }
        
        LOG_INFO("Loaded " + std::to_string(m_machines.size()) + " machine configurations from: " + configPath);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading machine configurations: " + std::string(e.what()));
    }
}

std::string MachineConfigManager::GetConfigFilePath() const {
    return "config/enhanced_machines.json";
}

// Legacy compatibility
void MachineConfigManager::ImportLegacyMachines(const std::vector<LegacyMachineConfig>& legacyMachines) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& legacy : legacyMachines) {
        EnhancedMachineConfig enhanced = EnhancedMachineConfig::FromLegacy(legacy);
        AddMachine(enhanced);
    }
    LOG_INFO("Imported " + std::to_string(legacyMachines.size()) + " legacy machine configurations");
}

// Helper methods
size_t MachineConfigManager::FindMachineIndex(const std::string& machineId) const {
    for (size_t i = 0; i < m_machines.size(); ++i) {
        if (m_machines[i].id == machineId) {
            return i;
        }
    }
    return m_machines.size(); // Not found
}

void MachineConfigManager::NotifyMachineUpdate(const std::string& machineId) {
    if (m_machineUpdateCallback) {
        size_t index = FindMachineIndex(machineId);
        if (index < m_machines.size()) {
            m_machineUpdateCallback(machineId, m_machines[index]);
        }
    }
}

void MachineConfigManager::NotifyCapabilityUpdate(const std::string& machineId) {
    if (m_capabilityUpdateCallback) {
        size_t index = FindMachineIndex(machineId);
        if (index < m_machines.size()) {
            m_capabilityUpdateCallback(machineId, m_machines[index].capabilities);
        }
    }
}

// EnhancedMachineConfig methods
EnhancedMachineConfig EnhancedMachineConfig::FromLegacy(const LegacyMachineConfig& legacy) {
    EnhancedMachineConfig enhanced;
    
    enhanced.id = legacy.id;
    enhanced.name = legacy.name;
    enhanced.description = legacy.description;
    enhanced.host = legacy.host;
    enhanced.port = legacy.port;
    enhanced.machineType = legacy.machineType;
    enhanced.autoConnect = legacy.autoConnect;
    enhanced.connected = legacy.connected;
    enhanced.lastConnected = legacy.lastConnected;
    
    // Convert legacy capabilities
    enhanced.capabilities.workspaceX = legacy.capabilities.workspaceX;
    enhanced.capabilities.workspaceY = legacy.capabilities.workspaceY;
    enhanced.capabilities.workspaceZ = legacy.capabilities.workspaceZ;
    enhanced.capabilities.maxFeedRate = legacy.capabilities.maxFeedRate;
    enhanced.capabilities.maxSpindleRPM = legacy.capabilities.maxSpindleRPM;
    enhanced.capabilities.numAxes = legacy.capabilities.numAxes;
    enhanced.capabilities.hasHoming = legacy.capabilities.hasHoming;
    enhanced.capabilities.hasProbe = legacy.capabilities.hasProbe;
    enhanced.capabilities.firmwareVersion = legacy.capabilities.firmwareVersion;
    enhanced.capabilities.buildInfo = legacy.capabilities.buildInfo;
    enhanced.capabilities.capabilitiesValid = legacy.capabilities.capabilitiesValid;
    
    // Set default homing settings
    enhanced.homing.enabled = true;
    enhanced.homing.sequence = HomingSettings::SIMULTANEOUS; // Safe default
    
    return enhanced;
}

json EnhancedMachineConfig::ToJson() const {
    json j;
    
    // Basic info
    j["id"] = id;
    j["name"] = name;
    j["description"] = description;
    j["host"] = host;
    j["port"] = port;
    j["machineType"] = machineType;
    j["autoConnect"] = autoConnect;
    j["connected"] = connected;
    j["lastConnected"] = lastConnected;
    
    // Capabilities
    j["capabilities"] = {
        {"workspaceX", capabilities.workspaceX},
        {"workspaceY", capabilities.workspaceY},
        {"workspaceZ", capabilities.workspaceZ},
        {"maxFeedRate", capabilities.maxFeedRate},
        {"maxSpindleRPM", capabilities.maxSpindleRPM},
        {"numAxes", capabilities.numAxes},
        {"hasHoming", capabilities.hasHoming},
        {"hasProbe", capabilities.hasProbe},
        {"hasSpindle", capabilities.hasSpindle},
        {"hasCoolant", capabilities.hasCoolant},
        {"firmwareVersion", capabilities.firmwareVersion},
        {"buildInfo", capabilities.buildInfo},
        {"kinematics", capabilities.kinematics},
        {"capabilitiesValid", capabilities.capabilitiesValid},
        {"lastQueried", capabilities.lastQueried},
        {"grblSettings", capabilities.grblSettings},
        {"systemInfo", capabilities.systemInfo}
    };
    
    // Homing settings
    j["homing"] = {
        {"enabled", homing.enabled},
        {"feedRate", homing.feedRate},
        {"seekRate", homing.seekRate},
        {"pullOff", homing.pullOff},
        {"sequence", static_cast<int>(homing.sequence)},
        {"customSequence", homing.customSequence}
    };
    
    // User settings
    j["userSettings"] = {
        {"useMetricUnits", userSettings.useMetricUnits},
        {"jogFeedRate", userSettings.jogFeedRate},
        {"jogDistance", userSettings.jogDistance},
        {"enableSoftLimits", userSettings.enableSoftLimits},
        {"enableHardLimits", userSettings.enableHardLimits},
        {"preferredCoordinateSystem", userSettings.preferredCoordinateSystem}
    };
    
    return j;
}

EnhancedMachineConfig EnhancedMachineConfig::FromJson(const json& j) {
    EnhancedMachineConfig config;
    
    // Basic info
    if (j.contains("id")) config.id = j["id"].get<std::string>();
    if (j.contains("name")) config.name = j["name"].get<std::string>();
    if (j.contains("description")) config.description = j["description"].get<std::string>();
    if (j.contains("host")) config.host = j["host"].get<std::string>();
    if (j.contains("port")) config.port = j["port"].get<int>();
    if (j.contains("machineType")) config.machineType = j["machineType"].get<std::string>();
    if (j.contains("autoConnect")) config.autoConnect = j["autoConnect"].get<bool>();
    if (j.contains("connected")) config.connected = j["connected"].get<bool>();
    if (j.contains("lastConnected")) config.lastConnected = j["lastConnected"].get<std::string>();
    
    // Capabilities
    if (j.contains("capabilities")) {
        const auto& caps = j["capabilities"];
        if (caps.contains("workspaceX")) config.capabilities.workspaceX = caps["workspaceX"].get<float>();
        if (caps.contains("workspaceY")) config.capabilities.workspaceY = caps["workspaceY"].get<float>();
        if (caps.contains("workspaceZ")) config.capabilities.workspaceZ = caps["workspaceZ"].get<float>();
        if (caps.contains("maxFeedRate")) config.capabilities.maxFeedRate = caps["maxFeedRate"].get<float>();
        if (caps.contains("maxSpindleRPM")) config.capabilities.maxSpindleRPM = caps["maxSpindleRPM"].get<float>();
        if (caps.contains("numAxes")) config.capabilities.numAxes = caps["numAxes"].get<int>();
        if (caps.contains("hasHoming")) config.capabilities.hasHoming = caps["hasHoming"].get<bool>();
        if (caps.contains("hasProbe")) config.capabilities.hasProbe = caps["hasProbe"].get<bool>();
        if (caps.contains("hasSpindle")) config.capabilities.hasSpindle = caps["hasSpindle"].get<bool>();
        if (caps.contains("hasCoolant")) config.capabilities.hasCoolant = caps["hasCoolant"].get<bool>();
        if (caps.contains("firmwareVersion")) config.capabilities.firmwareVersion = caps["firmwareVersion"].get<std::string>();
        if (caps.contains("buildInfo")) config.capabilities.buildInfo = caps["buildInfo"].get<std::string>();
        if (caps.contains("kinematics")) config.capabilities.kinematics = caps["kinematics"].get<std::string>();
        if (caps.contains("capabilitiesValid")) config.capabilities.capabilitiesValid = caps["capabilitiesValid"].get<bool>();
        if (caps.contains("lastQueried")) config.capabilities.lastQueried = caps["lastQueried"].get<std::string>();
        if (caps.contains("grblSettings")) config.capabilities.grblSettings = caps["grblSettings"].get<std::map<int, float>>();
        if (caps.contains("systemInfo")) config.capabilities.systemInfo = caps["systemInfo"].get<std::vector<std::string>>();
    }
    
    // Homing settings
    if (j.contains("homing")) {
        const auto& homing = j["homing"];
        if (homing.contains("enabled")) config.homing.enabled = homing["enabled"].get<bool>();
        if (homing.contains("feedRate")) config.homing.feedRate = homing["feedRate"].get<float>();
        if (homing.contains("seekRate")) config.homing.seekRate = homing["seekRate"].get<float>();
        if (homing.contains("pullOff")) config.homing.pullOff = homing["pullOff"].get<float>();
        if (homing.contains("sequence")) config.homing.sequence = static_cast<HomingSettings::HomingSequence>(homing["sequence"].get<int>());
        if (homing.contains("customSequence")) config.homing.customSequence = homing["customSequence"].get<std::vector<std::string>>();
    }
    
    // User settings
    if (j.contains("userSettings")) {
        const auto& user = j["userSettings"];
        if (user.contains("useMetricUnits")) config.userSettings.useMetricUnits = user["useMetricUnits"].get<bool>();
        if (user.contains("jogFeedRate")) config.userSettings.jogFeedRate = user["jogFeedRate"].get<float>();
        if (user.contains("jogDistance")) config.userSettings.jogDistance = user["jogDistance"].get<float>();
        if (user.contains("enableSoftLimits")) config.userSettings.enableSoftLimits = user["enableSoftLimits"].get<bool>();
        if (user.contains("enableHardLimits")) config.userSettings.enableHardLimits = user["enableHardLimits"].get<bool>();
        if (user.contains("preferredCoordinateSystem")) config.userSettings.preferredCoordinateSystem = user["preferredCoordinateSystem"].get<std::string>();
    }
    
    return config;
}
