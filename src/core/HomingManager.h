/**
 * core/HomingManager.h
 * Kinematics-aware homing manager for sequential command execution
 * Handles different machine types with appropriate homing sequences
 */

#pragma once

#include "MachineConfigManager.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

/**
 * Homing progress information for UI updates
 */
struct HomingProgress {
    enum State {
        IDLE,
        STARTING,
        HOMING_AXIS,
        WAITING_FOR_RESPONSE,
        COMPLETED,
        FAILED,
        CANCELLED
    };
    
    State state = IDLE;
    int currentStep = 0;          // Current step in sequence (0-based)
    int totalSteps = 0;           // Total steps in sequence
    std::string currentCommand = "";   // Current command being executed
    std::string currentAxis = "";      // Current axis being homed (Z, X, Y, etc.)
    std::string statusMessage = "";    // Human-readable status
    float progressPercent = 0.0f;      // 0-100% progress
    
    // Error information (when state == FAILED)
    std::string errorMessage = "";
    std::string failedCommand = "";
};

/**
 * Homing Manager - executes kinematics-aware homing sequences
 * Features:
 * - Sequential command execution with response waiting
 * - Progress tracking and UI callbacks
 * - Error handling and recovery
 * - Support for custom sequences with delays
 * - Thread-safe operation
 */
class HomingManager {
public:
    // Callback types for UI integration
    using ProgressCallback = std::function<void(const std::string& machineId, const HomingProgress& progress)>;
    using CommandSendCallback = std::function<bool(const std::string& machineId, const std::string& command)>;
    using LogCallback = std::function<void(const std::string& message, const std::string& level)>;
    
    // Singleton access
    static HomingManager& Instance();
    
    // Callback registration
    void SetProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }
    void SetCommandSendCallback(CommandSendCallback callback) { m_commandSendCallback = callback; }
    void SetLogCallback(LogCallback callback) { m_logCallback = callback; }
    
    // Main homing operations
    bool StartHomingSequence(const std::string& machineId);
    bool HomeSingleAxis(const std::string& machineId, const std::string& axis); // $HX, $HY, $HZ
    void CancelHoming(const std::string& machineId);
    
    // Response handling (called by CommunicationManager)
    void OnMachineResponse(const std::string& machineId, const std::string& response);
    
    // Status queries
    bool IsHoming(const std::string& machineId) const;
    HomingProgress GetHomingProgress(const std::string& machineId) const;
    
    // Configuration
    void SetResponseTimeout(int timeoutMs) { m_responseTimeoutMs = timeoutMs; }
    void SetInterCommandDelay(int delayMs) { m_interCommandDelayMs = delayMs; }
    
private:
    HomingManager() = default;
    ~HomingManager();
    
    // Non-copyable
    HomingManager(const HomingManager&) = delete;
    HomingManager& operator=(const HomingManager&) = delete;
    
    // Internal homing state for each machine
    struct HomingState {
        std::string machineId;
        HomingProgress progress;
        std::vector<std::string> commandSequence;
        std::atomic<bool> active{false};
        std::atomic<bool> cancelled{false};
        std::atomic<bool> waitingForResponse{false};
        std::chrono::steady_clock::time_point commandSentTime;
        std::string lastSentCommand;
        
        // Thread synchronization
        mutable std::mutex mutex;
        std::condition_variable responseReceived;
        bool gotResponse = false;
        std::string lastResponse;
    };
    
    // State management
    mutable std::mutex m_statesMutex;
    std::map<std::string, std::unique_ptr<HomingState>> m_homingStates;
    
    // Configuration
    int m_responseTimeoutMs = 10000;  // 10 second timeout for responses
    int m_interCommandDelayMs = 500;  // 500ms delay between commands
    
    // Callbacks
    ProgressCallback m_progressCallback;
    CommandSendCallback m_commandSendCallback;
    LogCallback m_logCallback;
    
    // Helper methods  
    HomingState* GetHomingState(const std::string& machineId);
    const HomingState* GetHomingState(const std::string& machineId) const;
    HomingState* GetOrCreateHomingState(const std::string& machineId);
    void RemoveHomingState(const std::string& machineId);
    
    // Sequence generation
    std::vector<std::string> GenerateHomingSequence(const EnhancedMachineConfig& config);
    std::vector<std::string> GenerateSimultaneousSequence();
    std::vector<std::string> GenerateSequentialZXYSequence();
    std::vector<std::string> GenerateSequentialZYXSequence();
    std::vector<std::string> GenerateCustomSequence(const std::vector<std::string>& customCommands);
    
    // Command execution
    void ExecuteHomingSequence(const std::string& machineId);
    bool SendHomingCommand(HomingState* state, const std::string& command);
    bool WaitForResponse(HomingState* state);
    void ProcessDelayCommand(const std::string& command); // For "G4 P500" style delays
    
    // Progress management
    void UpdateProgress(HomingState* state, HomingProgress::State newState, const std::string& message = "");
    void NotifyProgress(const std::string& machineId, const HomingProgress& progress);
    void LogMessage(const std::string& message, const std::string& level = "INFO");
    
    // Response analysis
    bool IsResponseOK(const std::string& response);
    bool IsResponseError(const std::string& response);
    std::string ExtractErrorMessage(const std::string& response);
    
    // Axis extraction from commands
    std::string ExtractAxisFromCommand(const std::string& command);
    std::string FormatProgressMessage(const HomingProgress& progress);
};

/**
 * RAII helper for automatic homing cancellation
 * Useful for UI dialogs that want to cancel homing when dismissed
 */
class HomingGuard {
public:
    HomingGuard(const std::string& machineId) : m_machineId(machineId), m_cancelled(false) {}
    
    ~HomingGuard() {
        if (!m_cancelled) {
            Cancel();
        }
    }
    
    void Cancel() {
        if (!m_cancelled) {
            HomingManager::Instance().CancelHoming(m_machineId);
            m_cancelled = true;
        }
    }
    
    void Release() {
        m_cancelled = true;
    }
    
private:
    std::string m_machineId;
    bool m_cancelled;
};
