/**
 * core/HomingManager.cpp
 * Implementation of kinematics-aware homing manager
 */

#include "HomingManager.h"
#include "SimpleLogger.h"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <regex>

// Singleton instance
HomingManager& HomingManager::Instance() {
    static HomingManager instance;
    return instance;
}

HomingManager::~HomingManager() {
    // Cancel all active homing operations
    std::lock_guard<std::mutex> lock(m_statesMutex);
    for (auto& [machineId, state] : m_homingStates) {
        if (state->active) {
            state->cancelled = true;
        }
    }
    m_homingStates.clear();
}

// Main homing operations
bool HomingManager::StartHomingSequence(const std::string& machineId) {
    // Get machine configuration
    EnhancedMachineConfig config = MachineConfigManager::Instance().GetMachine(machineId);
    if (config.id.empty()) {
        LogMessage("Cannot start homing: Machine not found: " + machineId, "ERROR");
        return false;
    }
    
    if (!config.homing.enabled) {
        LogMessage("Cannot start homing: Homing is disabled for machine: " + config.name, "WARN");
        return false;
    }
    
    // Check if already homing
    if (IsHoming(machineId)) {
        LogMessage("Cannot start homing: Already homing machine: " + config.name, "WARN");
        return false;
    }
    
    // Generate homing sequence based on kinematics
    std::vector<std::string> sequence = GenerateHomingSequence(config);
    if (sequence.empty()) {
        LogMessage("Cannot start homing: No homing sequence generated for machine: " + config.name, "ERROR");
        return false;
    }
    
    // Create/get homing state
    HomingState* state = GetOrCreateHomingState(machineId);
    if (!state) {
        LogMessage("Cannot start homing: Failed to create homing state for machine: " + config.name, "ERROR");
        return false;
    }
    
    // Initialize homing state
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->commandSequence = sequence;
        state->active = true;
        state->cancelled = false;
        state->waitingForResponse = false;
        state->gotResponse = false;
        state->lastResponse.clear();
        state->lastSentCommand.clear();
        
        state->progress = HomingProgress{};
        state->progress.state = HomingProgress::STARTING;
        state->progress.totalSteps = static_cast<int>(sequence.size());
        state->progress.currentStep = 0;
        state->progress.statusMessage = FormatProgressMessage(state->progress);
    }
    
    // Log homing start
    std::string sequenceStr = HomingSettings::SequenceToString(config.homing.sequence);
    LogMessage("Starting " + sequenceStr + " homing sequence for machine: " + config.name, "INFO");
    
    // Start execution in separate thread
    std::thread homingThread(&HomingManager::ExecuteHomingSequence, this, machineId);
    homingThread.detach();
    
    return true;
}

bool HomingManager::HomeSingleAxis(const std::string& machineId, const std::string& axis) {
    // Get machine configuration
    EnhancedMachineConfig config = MachineConfigManager::Instance().GetMachine(machineId);
    if (config.id.empty()) {
        LogMessage("Cannot home axis: Machine not found: " + machineId, "ERROR");
        return false;
    }
    
    if (!config.homing.enabled) {
        LogMessage("Cannot home axis: Homing is disabled for machine: " + config.name, "WARN");
        return false;
    }
    
    // Check if already homing
    if (IsHoming(machineId)) {
        LogMessage("Cannot home axis: Already homing machine: " + config.name, "WARN");
        return false;
    }
    
    // Validate axis
    if (axis.empty() || (axis != "X" && axis != "Y" && axis != "Z" && axis != "A" && axis != "B" && axis != "C")) {
        LogMessage("Cannot home axis: Invalid axis specified: " + axis, "ERROR");
        return false;
    }
    
    // Create single axis homing sequence
    std::vector<std::string> sequence = {"$H" + axis};
    
    // Create/get homing state
    HomingState* state = GetOrCreateHomingState(machineId);
    if (!state) {
        LogMessage("Cannot home axis: Failed to create homing state for machine: " + config.name, "ERROR");
        return false;
    }
    
    // Initialize homing state
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->commandSequence = sequence;
        state->active = true;
        state->cancelled = false;
        state->waitingForResponse = false;
        state->gotResponse = false;
        state->lastResponse.clear();
        state->lastSentCommand.clear();
        
        state->progress = HomingProgress{};
        state->progress.state = HomingProgress::STARTING;
        state->progress.totalSteps = 1;
        state->progress.currentStep = 0;
        state->progress.statusMessage = "Homing " + axis + " axis...";
    }
    
    LogMessage("Starting single axis homing for " + axis + " axis on machine: " + config.name, "INFO");
    
    // Start execution in separate thread
    std::thread homingThread(&HomingManager::ExecuteHomingSequence, this, machineId);
    homingThread.detach();
    
    return true;
}

void HomingManager::CancelHoming(const std::string& machineId) {
    HomingState* state = GetHomingState(machineId);
    if (state && state->active) {
        state->cancelled = true;
        
        // Wake up any waiting threads
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->gotResponse = true;
            state->lastResponse = "cancelled";
        }
        state->responseReceived.notify_all();
        
        LogMessage("Homing cancelled for machine: " + machineId, "INFO");
    }
}

// Response handling
void HomingManager::OnMachineResponse(const std::string& machineId, const std::string& response) {
    HomingState* state = GetHomingState(machineId);
    if (!state || !state->active || !state->waitingForResponse) {
        return; // Not actively homing or not waiting for response
    }
    
    // Store response and notify waiting thread
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->lastResponse = response;
        state->gotResponse = true;
    }
    state->responseReceived.notify_all();
}

// Status queries
bool HomingManager::IsHoming(const std::string& machineId) const {
    const HomingState* state = GetHomingState(machineId);
    return state && state->active && !state->cancelled;
}

HomingProgress HomingManager::GetHomingProgress(const std::string& machineId) const {
    const HomingState* state = GetHomingState(machineId);
    if (state) {
        std::lock_guard<std::mutex> lock(state->mutex);
        return state->progress;
    }
    return HomingProgress{};
}

// Helper methods
HomingManager::HomingState* HomingManager::GetHomingState(const std::string& machineId) {
    std::lock_guard<std::mutex> lock(m_statesMutex);
    auto it = m_homingStates.find(machineId);
    return (it != m_homingStates.end()) ? it->second.get() : nullptr;
}

const HomingManager::HomingState* HomingManager::GetHomingState(const std::string& machineId) const {
    std::lock_guard<std::mutex> lock(m_statesMutex);
    auto it = m_homingStates.find(machineId);
    return (it != m_homingStates.end()) ? it->second.get() : nullptr;
}

HomingManager::HomingState* HomingManager::GetOrCreateHomingState(const std::string& machineId) {
    std::lock_guard<std::mutex> lock(m_statesMutex);
    auto it = m_homingStates.find(machineId);
    if (it != m_homingStates.end()) {
        return it->second.get();
    }
    
    auto state = std::make_unique<HomingState>();
    state->machineId = machineId;
    HomingState* ptr = state.get();
    m_homingStates[machineId] = std::move(state);
    return ptr;
}

void HomingManager::RemoveHomingState(const std::string& machineId) {
    std::lock_guard<std::mutex> lock(m_statesMutex);
    m_homingStates.erase(machineId);
}

// Sequence generation
std::vector<std::string> HomingManager::GenerateHomingSequence(const EnhancedMachineConfig& config) {
    switch (config.homing.sequence) {
        case HomingSettings::SIMULTANEOUS:
            return GenerateSimultaneousSequence();
        case HomingSettings::SEQUENTIAL_ZXY:
            return GenerateSequentialZXYSequence();
        case HomingSettings::SEQUENTIAL_ZYX:
            return GenerateSequentialZYXSequence();
        case HomingSettings::CUSTOM:
            return GenerateCustomSequence(config.homing.customSequence);
        default:
            LogMessage("Unknown homing sequence type, using simultaneous", "WARN");
            return GenerateSimultaneousSequence();
    }
}

std::vector<std::string> HomingManager::GenerateSimultaneousSequence() {
    return {"$H"}; // Home all axes simultaneously
}

std::vector<std::string> HomingManager::GenerateSequentialZXYSequence() {
    return {"$HZ", "$HX", "$HY"}; // CoreXY sequence: Z first, then X, then Y
}

std::vector<std::string> HomingManager::GenerateSequentialZYXSequence() {
    return {"$HZ", "$HY", "$HX"}; // Alternative sequence: Z first, then Y, then X
}

std::vector<std::string> HomingManager::GenerateCustomSequence(const std::vector<std::string>& customCommands) {
    if (customCommands.empty()) {
        LogMessage("Custom homing sequence is empty, using simultaneous", "WARN");
        return GenerateSimultaneousSequence();
    }
    return customCommands;
}

// Command execution
void HomingManager::ExecuteHomingSequence(const std::string& machineId) {
    HomingState* state = GetHomingState(machineId);
    if (!state || !state->active) {
        return;
    }
    
    try {
        UpdateProgress(state, HomingProgress::STARTING, "Initializing homing sequence...");
        
        // Execute each command in sequence
        for (size_t i = 0; i < state->commandSequence.size() && state->active && !state->cancelled; ++i) {
            const std::string& command = state->commandSequence[i];
            
            // Update progress
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->progress.currentStep = static_cast<int>(i);
                state->progress.progressPercent = (static_cast<float>(i) / state->commandSequence.size()) * 100.0f;
                state->progress.currentCommand = command;
                state->progress.currentAxis = ExtractAxisFromCommand(command);
                state->progress.statusMessage = FormatProgressMessage(state->progress);
            }
            
            // Check for delay commands
            if (command.find("G4") == 0) {
                ProcessDelayCommand(command);
                continue;
            }
            
            // Send homing command
            UpdateProgress(state, HomingProgress::HOMING_AXIS, "Sending: " + command);
            
            if (!SendHomingCommand(state, command)) {
                UpdateProgress(state, HomingProgress::FAILED, "Failed to send command: " + command);
                break;
            }
            
            // Wait for response
            UpdateProgress(state, HomingProgress::WAITING_FOR_RESPONSE, "Waiting for response...");
            
            if (!WaitForResponse(state)) {
                if (state->cancelled) {
                    UpdateProgress(state, HomingProgress::CANCELLED, "Homing sequence cancelled");
                } else {
                    UpdateProgress(state, HomingProgress::FAILED, "Timeout or error waiting for response");
                }
                break;
            }
            
            // Check if we got an error response
            if (IsResponseError(state->lastResponse)) {
                std::string errorMsg = ExtractErrorMessage(state->lastResponse);
                UpdateProgress(state, HomingProgress::FAILED, "Homing error: " + errorMsg);
                {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->progress.errorMessage = errorMsg;
                    state->progress.failedCommand = command;
                }
                break;
            }
            
            // Add inter-command delay (except for last command)
            if (i < state->commandSequence.size() - 1 && m_interCommandDelayMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(m_interCommandDelayMs));
            }
        }
        
        // Final status update
        if (state->cancelled) {
            UpdateProgress(state, HomingProgress::CANCELLED, "Homing sequence cancelled");
        } else if (state->progress.state != HomingProgress::FAILED) {
            UpdateProgress(state, HomingProgress::COMPLETED, "Homing sequence completed successfully");
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->progress.progressPercent = 100.0f;
            }
        }
        
        // Mark as inactive
        state->active = false;
        
        // Log completion
        EnhancedMachineConfig config = MachineConfigManager::Instance().GetMachine(machineId);
        if (state->progress.state == HomingProgress::COMPLETED) {
            LogMessage("Homing sequence completed successfully for machine: " + config.name, "INFO");
        } else if (state->progress.state == HomingProgress::CANCELLED) {
            LogMessage("Homing sequence cancelled for machine: " + config.name, "INFO");
        } else {
            LogMessage("Homing sequence failed for machine: " + config.name + " - " + state->progress.errorMessage, "ERROR");
        }
        
    } catch (const std::exception& e) {
        UpdateProgress(state, HomingProgress::FAILED, "Exception during homing: " + std::string(e.what()));
        state->active = false;
        LogMessage("Exception during homing sequence: " + std::string(e.what()), "ERROR");
    }
}

bool HomingManager::SendHomingCommand(HomingState* state, const std::string& command) {
    if (!m_commandSendCallback) {
        LogMessage("No command send callback registered", "ERROR");
        return false;
    }
    
    // Mark as waiting for response
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->waitingForResponse = true;
        state->gotResponse = false;
        state->lastResponse.clear();
        state->lastSentCommand = command;
        state->commandSentTime = std::chrono::steady_clock::now();
    }
    
    // Send command
    bool success = m_commandSendCallback(state->machineId, command);
    
    if (!success) {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->waitingForResponse = false;
    }
    
    return success;
}

bool HomingManager::WaitForResponse(HomingState* state) {
    std::unique_lock<std::mutex> lock(state->mutex);
    
    // Wait for response or timeout
    bool gotResponse = state->responseReceived.wait_for(lock, 
        std::chrono::milliseconds(m_responseTimeoutMs),
        [state]() { return state->gotResponse || state->cancelled; });
    
    state->waitingForResponse = false;
    
    return gotResponse && !state->cancelled;
}

void HomingManager::ProcessDelayCommand(const std::string& command) {
    // Parse G4 P<milliseconds> command
    std::regex delayRegex(R"(G4\s+P(\d+))", std::regex_constants::icase);
    std::smatch match;
    
    if (std::regex_search(command, match, delayRegex)) {
        int delayMs = std::stoi(match[1].str());
        LogMessage("Processing delay command: " + command + " (" + std::to_string(delayMs) + "ms)", "DEBUG");
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    } else {
        LogMessage("Invalid delay command format: " + command, "WARN");
    }
}

// Progress management
void HomingManager::UpdateProgress(HomingState* state, HomingProgress::State newState, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->progress.state = newState;
        if (!message.empty()) {
            state->progress.statusMessage = message;
        }
    }
    
    NotifyProgress(state->machineId, state->progress);
}

void HomingManager::NotifyProgress(const std::string& machineId, const HomingProgress& progress) {
    if (m_progressCallback) {
        m_progressCallback(machineId, progress);
    }
}

void HomingManager::LogMessage(const std::string& message, const std::string& level) {
    if (m_logCallback) {
        m_logCallback(message, level);
    }
    
    // Also log to SimpleLogger if available
    if (level == "ERROR") {
        LOG_ERROR(message);
    } else if (level == "WARN") {
        LOG_WARNING(message);
    } else {
        LOG_INFO(message);
    }
}

// Response analysis
bool HomingManager::IsResponseOK(const std::string& response) {
    std::string lowerResponse = response;
    std::transform(lowerResponse.begin(), lowerResponse.end(), lowerResponse.begin(), ::tolower);
    return lowerResponse.find("ok") != std::string::npos;
}

bool HomingManager::IsResponseError(const std::string& response) {
    std::string lowerResponse = response;
    std::transform(lowerResponse.begin(), lowerResponse.end(), lowerResponse.begin(), ::tolower);
    return lowerResponse.find("error") != std::string::npos || lowerResponse.find("alarm") != std::string::npos;
}

std::string HomingManager::ExtractErrorMessage(const std::string& response) {
    // Try to extract meaningful error message
    if (response.find(":") != std::string::npos) {
        return response.substr(response.find(":") + 1);
    }
    return response;
}

// Axis extraction from commands
std::string HomingManager::ExtractAxisFromCommand(const std::string& command) {
    if (command.find("$H") == 0 && command.length() > 2) {
        return command.substr(2, 1); // Extract single character after $H
    } else if (command == "$H") {
        return "All";
    } else if (command.find("G4") == 0) {
        return ""; // Delay command, no axis
    }
    return "";
}

std::string HomingManager::FormatProgressMessage(const HomingProgress& progress) {
    switch (progress.state) {
        case HomingProgress::IDLE:
            return "Homing idle";
        case HomingProgress::STARTING:
            return "Starting homing sequence...";
        case HomingProgress::HOMING_AXIS:
            if (!progress.currentAxis.empty() && progress.currentAxis != "All") {
                return "Homing " + progress.currentAxis + " axis...";
            } else if (progress.currentAxis == "All") {
                return "Homing all axes...";
            } else {
                return "Executing homing command...";
            }
        case HomingProgress::WAITING_FOR_RESPONSE:
            return "Waiting for machine response...";
        case HomingProgress::COMPLETED:
            return "Homing completed successfully";
        case HomingProgress::FAILED:
            return "Homing failed: " + progress.errorMessage;
        case HomingProgress::CANCELLED:
            return "Homing cancelled by user";
        default:
            return "Unknown homing state";
    }
}
