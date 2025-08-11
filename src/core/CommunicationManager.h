/**
 * core/CommunicationManager.h
 * Manages real communication between the GUI and FluidNC machines
 * Integrates the existing FluidNCClient with the GUI components
 */

#pragma once

#include "FluidNCClient.h"
#include <string>
#include <memory>
#include <map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

// Forward declarations
struct MachineConfig;

/**
 * Communication Manager handles real FluidNC communication
 * Integrates with GUI panels and provides real machine connectivity
 */
class CommunicationManager
{
public:
    // Callback types for GUI integration
    using MessageCallback = std::function<void(const std::string& machineId, const std::string& message, const std::string& level)>;
    using CommandSentCallback = std::function<void(const std::string& machineId, const std::string& command)>;
    using ResponseReceivedCallback = std::function<void(const std::string& machineId, const std::string& response)>;
    using ConnectionStatusCallback = std::function<void(const std::string& machineId, bool connected)>;
    using DROUpdateCallback = std::function<void(const std::string& machineId, const std::vector<float>& mpos, const std::vector<float>& wpos)>;

    static CommunicationManager& Instance();
    
    // Connection management
    bool ConnectMachine(const std::string& machineId, const std::string& host, int port);
    bool DisconnectMachine(const std::string& machineId);
    bool IsConnected(const std::string& machineId) const;
    
    // Command sending
    bool SendCommand(const std::string& machineId, const std::string& command);
    
    // Callback registration
    void SetMessageCallback(MessageCallback callback) { m_messageCallback = callback; }
    void SetCommandSentCallback(CommandSentCallback callback) { m_commandSentCallback = callback; }
    void SetResponseReceivedCallback(ResponseReceivedCallback callback) { m_responseReceivedCallback = callback; }
    void SetConnectionStatusCallback(ConnectionStatusCallback callback) { m_connectionStatusCallback = callback; }
    void SetDROUpdateCallback(DROUpdateCallback callback) { m_droUpdateCallback = callback; }
    
    // Get current DRO data
    std::vector<float> GetMachinePosition(const std::string& machineId) const;
    std::vector<float> GetWorkPosition(const std::string& machineId) const;
    
    // Cleanup
    void DisconnectAll();

private:
    CommunicationManager() = default;
    ~CommunicationManager();
    
    // Connection data structure
    struct ConnectionInfo {
        std::unique_ptr<FluidNCClient> client;
        std::string machineId;
        std::string host;
        int port;
        std::atomic<bool> connected{false};
    };
    
    // FluidNC client callbacks (called from FluidNC threads)
    void OnConnect(const std::string& machineId);
    void OnDisconnect(const std::string& machineId);
    void OnResponse(const std::string& machineId, const std::string& response);
    void OnDROUpdate(const std::string& machineId, const std::vector<float>& mpos, const std::vector<float>& wpos);
    
    // Thread-safe client management
    mutable std::mutex m_connectionsMutex;
    std::map<std::string, std::unique_ptr<ConnectionInfo>> m_connections;
    
    // GUI callbacks
    MessageCallback m_messageCallback;
    CommandSentCallback m_commandSentCallback;
    ResponseReceivedCallback m_responseReceivedCallback;
    ConnectionStatusCallback m_connectionStatusCallback;
    DROUpdateCallback m_droUpdateCallback;
    
    // Non-copyable
    CommunicationManager(const CommunicationManager&) = delete;
    CommunicationManager& operator=(const CommunicationManager&) = delete;
};
