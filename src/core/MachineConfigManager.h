/**
 * core/MachineConfigManager.h
 * Enhanced machine configuration management with kinematics-aware homing support
 * Centralized singleton for all machine-specific information and capabilities
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>
#include <json.hpp>

using json = nlohmann::json;

/**
 * Kinematics-aware homing configuration
 * Different machine types require different homing sequences:
 * - Cartesian: All axes simultaneously ($H)
 * - CoreXY: Sequential Z->X->Y ($HZ, $HX, $HY) to prevent motor interference
 * - Delta: Simultaneous ($H) - towers move together naturally
 * - SCARA/Custom: User-defined sequences with delays
 */
struct HomingSettings {
    bool enabled = true;
    float feedRate = 500.0f;    // mm/min - homing feed rate
    float seekRate = 2500.0f;   // mm/min - initial seek rate
    float pullOff = 1.0f;       // mm - distance to pull off switches
    
    // Kinematics-specific homing sequences
    enum HomingSequence {
        SIMULTANEOUS,   // Standard Cartesian: $H
        SEQUENTIAL_ZXY, // CoreXY: $HZ -> $HX -> $HY  
        SEQUENTIAL_ZYX, // Alternative: $HZ -> $HY -> $HX
        CUSTOM         // User-defined sequence
    };
    
    HomingSequence sequence = SIMULTANEOUS;
    std::vector<std::string> customSequence; // For complex kinematics
    
    // Convert enum to string for UI display
    static std::string SequenceToString(HomingSequence seq) {
        switch (seq) {
            case SIMULTANEOUS: return "Simultaneous (Cartesian)";
            case SEQUENTIAL_ZXY: return "Sequential Z->X->Y (CoreXY)";
            case SEQUENTIAL_ZYX: return "Sequential Z->Y->X (Alternative)";
            case CUSTOM: return "Custom Sequence";
            default: return "Unknown";
        }
    }
    
    static HomingSequence SequenceFromString(const std::string& str) {
        if (str.find("SEQUENTIAL_ZXY") != std::string::npos || str.find("Z->X->Y") != std::string::npos) return SEQUENTIAL_ZXY;
        if (str.find("SEQUENTIAL_ZYX") != std::string::npos || str.find("Z->Y->X") != std::string::npos) return SEQUENTIAL_ZYX;
        if (str.find("CUSTOM") != std::string::npos || str.find("Custom") != std::string::npos) return CUSTOM;
        return SIMULTANEOUS;
    }
};

/**
 * Machine capabilities discovered from FluidNC machine
 * Populated automatically when machine connects
 */
struct MachineCapabilities {
    // Workspace bounds (travel limits)
    float workspaceX = 0.0f, workspaceY = 0.0f, workspaceZ = 0.0f;
    
    // Performance limits
    float maxFeedRate = 1000.0f;
    float maxSpindleRPM = 24000.0f;
    
    // Axes and features
    int numAxes = 3;
    bool hasHoming = false;
    bool hasProbe = false;
    bool hasSpindle = false;
    bool hasCoolant = false;
    
    // Machine identification
    std::string firmwareVersion = "";
    std::string buildInfo = "";
    std::string kinematics = "Cartesian";  // Cartesian, CoreXY, Delta, SCARA
    
    // GRBL settings ($$)
    std::map<int, float> grblSettings;
    
    // System info ($I)
    std::vector<std::string> systemInfo;
    
    // Validation
    bool capabilitiesValid = false;  // True when data has been queried from machine
    std::string lastQueried = "";    // Timestamp of last capability query
};

/**
 * Comprehensive machine configuration
 * Includes connection settings, discovered capabilities, and kinematics-aware homing
 */
struct EnhancedMachineConfig {
    // Basic identification
    std::string id;
    std::string name;
    std::string description;
    
    // Connection settings
    std::string host;
    int port = 23;
    std::string machineType = "FluidNC";
    bool autoConnect = false;
    
    // Connection status
    bool connected = false;
    std::string lastConnected = "Never";
    
    // Machine capabilities (auto-discovered)
    MachineCapabilities capabilities;
    
    // Kinematics-aware homing configuration
    HomingSettings homing;
    
    // User preferences and overrides
    struct UserSettings {
        bool useMetricUnits = true;
        float jogFeedRate = 1000.0f;
        float jogDistance = 1.0f;
        bool enableSoftLimits = true;
        bool enableHardLimits = true;
        std::string preferredCoordinateSystem = "G54";
    } userSettings;
    
    // Default constructor
    EnhancedMachineConfig() = default;
    
    // Convert from legacy MachineConfig (for compatibility)
    static EnhancedMachineConfig FromLegacy(const struct LegacyMachineConfig& legacy);
    
    // JSON serialization
    json ToJson() const;
    static EnhancedMachineConfig FromJson(const json& j);
};

/**
 * Singleton Machine Configuration Manager
 * Centralized management of all machine configurations and capabilities
 * All panels can access machine information through this manager
 */
class MachineConfigManager {
public:
    // Singleton access
    static MachineConfigManager& Instance();
    
    // Machine management
    std::vector<EnhancedMachineConfig> GetAllMachines() const;
    EnhancedMachineConfig GetMachine(const std::string& machineId) const;
    void AddMachine(const EnhancedMachineConfig& machine);
    void UpdateMachine(const std::string& machineId, const EnhancedMachineConfig& machine);
    void RemoveMachine(const std::string& machineId);
    
    // Active machine management
    void SetActiveMachine(const std::string& machineId);
    std::string GetActiveMachineId() const;
    EnhancedMachineConfig GetActiveMachine() const;
    bool HasActiveMachine() const;
    
    // Machine capability management
    void UpdateMachineCapabilities(const std::string& machineId, const MachineCapabilities& capabilities);
    MachineCapabilities GetMachineCapabilities(const std::string& machineId) const;
    
    // Kinematics detection and auto-configuration
    std::string DetectKinematics(const std::map<int, float>& grblSettings, const std::vector<std::string>& systemInfo) const;
    void AutoConfigureHoming(const std::string& machineId, const std::string& kinematics);
    
    // Connection status updates
    void UpdateConnectionStatus(const std::string& machineId, bool connected, const std::string& timestamp = "");
    
    // Event callbacks for UI updates
    using MachineUpdateCallback = std::function<void(const std::string& machineId, const EnhancedMachineConfig& machine)>;
    using CapabilityUpdateCallback = std::function<void(const std::string& machineId, const MachineCapabilities& capabilities)>;
    
    void SetMachineUpdateCallback(MachineUpdateCallback callback) { m_machineUpdateCallback = callback; }
    void SetCapabilityUpdateCallback(CapabilityUpdateCallback callback) { m_capabilityUpdateCallback = callback; }
    
    // Persistence
    void SaveToFile();
    void LoadFromFile();
    std::string GetConfigFilePath() const;
    
    // Legacy compatibility
    void ImportLegacyMachines(const std::vector<struct LegacyMachineConfig>& legacyMachines);
    
private:
    MachineConfigManager() = default;
    ~MachineConfigManager() = default;
    
    // Non-copyable
    MachineConfigManager(const MachineConfigManager&) = delete;
    MachineConfigManager& operator=(const MachineConfigManager&) = delete;
    
    // Internal data
    std::vector<EnhancedMachineConfig> m_machines;
    std::string m_activeMachineId;
    mutable std::mutex m_mutex;
    
    // Callbacks
    MachineUpdateCallback m_machineUpdateCallback;
    CapabilityUpdateCallback m_capabilityUpdateCallback;
    
    // Helper methods
    size_t FindMachineIndex(const std::string& machineId) const;
    void NotifyMachineUpdate(const std::string& machineId);
    void NotifyCapabilityUpdate(const std::string& machineId);
};

// Legacy compatibility structure (matches existing MachineManagerPanel::MachineConfig)
struct LegacyMachineConfig {
    std::string id;
    std::string name;
    std::string description;
    std::string host;
    int port;
    std::string machineType;
    bool connected;
    std::string lastConnected;
    bool autoConnect;
    
    struct MachineCapabilities {
        float workspaceX, workspaceY, workspaceZ;
        float maxFeedRate;
        float maxSpindleRPM;
        int numAxes;
        bool hasHoming;
        bool hasProbe;
        std::string firmwareVersion;
        std::string buildInfo;
        bool capabilitiesValid;
    } capabilities;
};
