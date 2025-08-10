/**
 * core/ConnectionManager.h
 * Multi-protocol connection manager supporting Telnet, USB, UART
 * Manages multiple machine connections simultaneously
 */

#pragma once

#include "StateManager.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

// Forward declarations
class IConnection;
class TelnetConnection;
class USBConnection;
class UARTConnection;

// Connection status
enum class ConnectionStatus {
    Disconnected,
    Connecting,
    Connected,
    Error
};

// Machine status information
struct MachineStatus {
    std::string machineId;
    ConnectionStatus status = ConnectionStatus::Disconnected;
    std::vector<float> machinePos = {0.0f, 0.0f, 0.0f};
    std::vector<float> workPos = {0.0f, 0.0f, 0.0f};
    std::string currentState = "Idle";  // Idle, Run, Hold, Jog, Alarm, etc.
    float feedRate = 0.0f;
    float spindleSpeed = 0.0f;
    std::string lastError;
};

/**
 * ConnectionManager handles multiple CNC machine connections
 * Supports automatic switching between machines and connection management
 */
class ConnectionManager
{
public:
    // Callback types
    using StatusCallback = std::function<void(const std::string& machineId, const MachineStatus& status)>;
    using ResponseCallback = std::function<void(const std::string& machineId, const std::string& response)>;
    using ErrorCallback = std::function<void(const std::string& machineId, const std::string& error)>;
    
    ConnectionManager();
    ~ConnectionManager();
    
    // Machine management
    bool addMachine(const MachineConfig& config);
    bool removeMachine(const std::string& machineId);
    bool updateMachine(const MachineConfig& config);
    std::vector<std::string> getMachineIds() const;
    MachineConfig getMachineConfig(const std::string& machineId) const;
    
    // Connection management
    bool connectMachine(const std::string& machineId);
    bool disconnectMachine(const std::string& machineId);
    void disconnectAll();
    bool isConnected(const std::string& machineId) const;
    ConnectionStatus getConnectionStatus(const std::string& machineId) const;
    
    // Active machine management
    void setActiveMachine(const std::string& machineId);
    std::string getActiveMachine() const;
    
    // Command sending
    bool sendCommand(const std::string& machineId, const std::string& command);
    bool sendCommandToActive(const std::string& command);
    bool sendFile(const std::string& machineId, const std::vector<std::string>& gcode);
    void emergencyStop(const std::string& machineId = ""); // Empty = all machines
    
    // Status and monitoring
    MachineStatus getMachineStatus(const std::string& machineId) const;
    std::vector<MachineStatus> getAllStatuses() const;
    
    // Callback registration
    void setStatusCallback(StatusCallback callback) { m_statusCallback = callback; }
    void setResponseCallback(ResponseCallback callback) { m_responseCallback = callback; }
    void setErrorCallback(ErrorCallback callback) { m_errorCallback = callback; }
    
    // Auto-connect management
    void enableAutoConnect(bool enable);
    bool isAutoConnectEnabled() const { return m_autoConnect.load(); }
    
private:
    // Internal connection management
    std::unique_ptr<IConnection> createConnection(const MachineConfig& config);
    void connectionThread(const std::string& machineId);
    void updateMachineStatus(const std::string& machineId, const MachineStatus& status);
    
    // Thread-safe data structures
    mutable std::recursive_mutex m_machinesMutex;
    std::map<std::string, MachineConfig> m_machines;
    std::map<std::string, std::unique_ptr<IConnection>> m_connections;
    std::map<std::string, MachineStatus> m_statuses;
    std::map<std::string, std::thread> m_connectionThreads;
    
    std::string m_activeMachine;
    std::atomic<bool> m_autoConnect{false};
    std::atomic<bool> m_shutdown{false};
    
    // Callbacks
    StatusCallback m_statusCallback;
    ResponseCallback m_responseCallback;
    ErrorCallback m_errorCallback;
    
    // State manager reference
    StateManager& m_stateManager;
};

/**
 * Abstract base class for different connection types
 */
class IConnection
{
public:
    virtual ~IConnection() = default;
    
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool sendData(const std::string& data) = 0;
    virtual std::string receiveData(int timeoutMs = 100) = 0;
    virtual ConnectionStatus getStatus() const = 0;
    virtual std::string getLastError() const = 0;
    
protected:
    std::string m_lastError;
    std::atomic<ConnectionStatus> m_status{ConnectionStatus::Disconnected};
};
