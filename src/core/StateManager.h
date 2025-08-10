/**
 * core/StateManager.h
 * Comprehensive state management for multi-machine CNC control application
 * Handles: UI layouts, machine configurations, job settings, user preferences
 */

#pragma once

#include <json.hpp>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <filesystem>

using json = nlohmann::json;

// Machine connection types
enum class ConnectionType {
    Telnet,
    USB,
    UART
};

// Machine configuration structure
struct MachineConfig {
    std::string name;
    std::string id;  // Unique identifier
    ConnectionType type;
    std::string host;      // For telnet
    int port = 23;         // For telnet
    std::string device;    // For USB/UART
    int baudRate = 115200; // For UART
    bool autoConnect = false;
    
    // Machine-specific settings
    json machineSettings;  // Speeds, feeds, limits, etc.
};

// Window layout information
struct WindowLayout {
    std::string windowId;
    int x, y, width, height;
    bool visible = true;
    bool docked = true;
    bool maximized = false;
    std::string dockingSide = "center";  // left, right, top, bottom, center
};

// Job settings structure
struct JobSettings {
    std::string name;
    float feedRate = 1000.0f;
    float spindleSpeed = 10000.0f;
    float safeZ = 5.0f;
    float workZ = -1.0f;
    float depthPerPass = 0.5f;
    std::string material = "Wood";
    std::string toolType = "End Mill";
    float toolDiameter = 3.175f;
};

class StateManager
{
public:
    // Singleton pattern
    static StateManager& getInstance();
    
    // Prevent copying
    StateManager(const StateManager&) = delete;
    StateManager& operator=(const StateManager&) = delete;
    
    // General configuration API
    template<typename T>
    T getValue(const std::string& key, const T& defaultValue = T{});
    
    template<typename T>
    void setValue(const std::string& key, const T& value);
    
    // Machine management
    std::vector<MachineConfig> getMachines() const;
    void addMachine(const MachineConfig& machine);
    void updateMachine(const std::string& id, const MachineConfig& machine);
    void removeMachine(const std::string& id);
    MachineConfig getMachine(const std::string& id) const;
    void setActiveMachine(const std::string& id);
    std::string getActiveMachineId() const;
    
    // Window layout management
    std::vector<WindowLayout> getWindowLayouts() const;
    void saveWindowLayout(const WindowLayout& layout);
    WindowLayout getWindowLayout(const std::string& windowId) const;
    void resetWindowLayouts();  // Reset to defaults
    
    // Job settings management
    JobSettings getCurrentJobSettings() const;
    void setCurrentJobSettings(const JobSettings& settings);
    std::vector<JobSettings> getSavedJobProfiles() const;
    void saveJobProfile(const JobSettings& settings);
    void deleteJobProfile(const std::string& name);
    
    // File management
    void save();                    // Manual save to settings.json
    void saveRecovery();           // Fast dump for power-cut recovery
    void shutdown();               // Stop autosave thread and save
    
    // Configuration file paths
    std::string getSettingsFilePath() const;
    std::string getRecoveryFilePath() const;
    
private:
    StateManager();
    ~StateManager();
    
    void load();                   // Load from settings.json
    void autosaveLoop();          // Autosave thread function
    void createConfigDirs();      // Ensure config directory exists
    void initializeDefaults();    // Set up default configuration
    
    // Helper functions for nested key access
    json* getNestedValue(const std::string& key);
    void setNestedValue(const std::string& key, const json& value);
    std::vector<std::string> splitKey(const std::string& key);
    
    // JSON conversion helpers
    json machineConfigToJson(const MachineConfig& config) const;
    MachineConfig machineConfigFromJson(const json& j) const;
    json windowLayoutToJson(const WindowLayout& layout) const;
    WindowLayout windowLayoutFromJson(const json& j) const;
    json jobSettingsToJson(const JobSettings& settings) const;
    JobSettings jobSettingsFromJson(const json& j) const;
    
    mutable std::recursive_mutex m_mutex;
    json m_data;
    
    // Autosave thread
    std::thread m_autosaveThread;
    std::atomic<bool> m_stopAutosave;
    
    // File paths
    const std::filesystem::path m_configDir = "config";
    const std::filesystem::path m_settingsFile = m_configDir / "settings.json";
    const std::filesystem::path m_recoveryFile = m_configDir / "recovery.json";
    const std::filesystem::path m_machinesFile = m_configDir / "machines.json";
    const std::filesystem::path m_jobProfilesFile = m_configDir / "job_profiles.json";
};

// Utility function for connection type conversion
std::string connectionTypeToString(ConnectionType type);
ConnectionType connectionTypeFromString(const std::string& str);

// Template implementations must be in header
template<typename T>
T StateManager::getValue(const std::string& key, const T& defaultValue)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (key.empty()) {
        return defaultValue;
    }
    
    json* valuePtr = getNestedValue(key);
    if (valuePtr && !valuePtr->is_null()) {
        try {
            return valuePtr->get<T>();
        } catch (const json::exception&) {
            // Type conversion failed, return default
        }
    }
    
    return defaultValue;
}

template<typename T>
void StateManager::setValue(const std::string& key, const T& value)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    if (!key.empty()) {
        setNestedValue(key, json(value));
    }
}
