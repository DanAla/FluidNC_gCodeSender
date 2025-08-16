/**
 * core/CommunicationManager.cpp
 * Implementation of real communication between GUI and FluidNC machines
 */

#include "CommunicationManager.h"
#include "SimpleLogger.h"
#include "ErrorHandler.h"
#include "gui/UIQueue.h"

CommunicationManager& CommunicationManager::Instance()
{
    static CommunicationManager instance;
    return instance;
}

CommunicationManager::~CommunicationManager()
{
    DisconnectAll();
}

bool CommunicationManager::ConnectMachine(const std::string& machineId, const std::string& host, int port)
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    try {
        // Check if already connected
        auto it = m_connections.find(machineId);
        if (it != m_connections.end() && it->second->connected.load()) {
            LOG_INFO("Machine " + machineId + " is already connected");
            ErrorHandler::Instance().ReportWarning(
                "Connection Warning",
                "Machine " + machineId + " is already connected",
                "Host: " + host + "\nPort: " + std::to_string(port)
            );
            return true;
        }
        
        LOG_INFO("Creating new connection for machine: " + machineId);
        
        // Create new connection
        auto connectionInfo = std::make_unique<ConnectionInfo>();
        connectionInfo->machineId = machineId;
        connectionInfo->host = host;
        connectionInfo->port = port;
        connectionInfo->connected = false;
        
        // Store the connection info first so callbacks can access it
        m_connections[machineId] = std::move(connectionInfo);
        
        try {
            // Create FluidNC client with DRO callback
            m_connections[machineId]->client = std::make_unique<FluidNCClient>(
                host, port,
                [this, machineId](const std::vector<float>& mpos, const std::vector<float>& wpos) {
                    UIQueue::getInstance().push([this, machineId, mpos, wpos]() {
                        OnDROUpdate(machineId, mpos, wpos);
                    });
                }
            );
            
            // Set connection callbacks
            m_connections[machineId]->client->setOnConnectCallback([this, machineId]() {
                OnConnect(machineId);
            });
            
            m_connections[machineId]->client->setOnDisconnectCallback([this, machineId]() {
                OnDisconnect(machineId);
                
                UIQueue::getInstance().push([this, machineId]() {
                    std::lock_guard<std::mutex> lock(m_connectionsMutex);
                    auto it = m_connections.find(machineId);
                    if (it != m_connections.end()) {
                        ErrorHandler::Instance().ReportWarning(
                            "Connection Lost",
                            "Lost connection to machine " + machineId,
                            "The machine may be offline or experiencing network issues.\n\n"
                            "Host: " + it->second->host + "\n"
                            "Port: " + std::to_string(it->second->port)
                        );
                    }
                });
            });
            
            // Set response callback
            m_connections[machineId]->client->setResponseCallback([this, machineId](const std::string& response) {
                UIQueue::getInstance().push([this, machineId, response]() {
                    OnResponse(machineId, response);
                });
            });
            
            LOG_INFO("Starting connection attempt for machine: " + machineId);
            
            // Start the client (this will attempt connection)
            m_connections[machineId]->client->start();
            
            LOG_INFO("Connection attempt started for machine: " + machineId + " (" + host + ":" + std::to_string(port) + ")");
            
            return true;
            
        } catch (const std::exception& e) {
            // Clean up the connection info on failure
            m_connections.erase(machineId);
            throw; // Re-throw to be caught by outer try-catch
        }
        
    } catch (const std::exception& e) {
        ErrorHandler::Instance().ReportError(
            "Connection Error",
            "Failed to connect to machine " + machineId,
            "Host: " + host + "\n" 
            "Port: " + std::to_string(port) + "\n\n" 
            "Error: " + std::string(e.what()) + "\n\n" 
            "The machine may be offline or unreachable.\n\n" 
            "Please check:\n" 
            "1. Machine is powered on\n" 
            "2. Network connection is stable\n" 
            "3. IP address and port are correct"
        );
        return false;
    }
}

bool CommunicationManager::DisconnectMachine(const std::string& machineId)
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_connections.find(machineId);
    if (it != m_connections.end()) {
        LOG_INFO("Disconnecting machine: " + machineId);
        
        // Stop the client (this will disconnect and clean up threads)
        it->second->client->stop();
        it->second->connected = false;
        
        // Remove from connections map
        m_connections.erase(it);
        
        return true;
    }
    
    LOG_ERROR("Attempted to disconnect unknown machine: " + machineId);
    return false;
}

bool CommunicationManager::IsConnected(const std::string& machineId) const
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_connections.find(machineId);
    if (it != m_connections.end()) {
        return it->second->connected.load() && it->second->client->isConnected();
    }
    return false;
}

bool CommunicationManager::SendCommand(const std::string& machineId, const std::string& command)
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_connections.find(machineId);
    if (it != m_connections.end() && it->second->connected.load() && it->second->client->isConnected()) {
        // Log the sent command immediately
        if (m_commandSentCallback) {
            UIQueue::getInstance().push([this, machineId, command]() {
                m_commandSentCallback(machineId, command);
            });
        }
        
        // Send the command to the machine
        it->second->client->sendGCodeLine(command);
        
        LOG_INFO("Sent command to " + machineId + ": " + command);
        return true;
    } else {
        LOG_ERROR("Cannot send command to disconnected machine: " + machineId);
        
        // Notify GUI of error
        if (m_messageCallback) {
            UIQueue::getInstance().push([this, machineId]() {
                m_messageCallback(machineId, "Cannot send command - machine not connected", "ERROR");
            });
        }
        
        return false;
    }
}

std::vector<float> CommunicationManager::GetMachinePosition(const std::string& machineId) const
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_connections.find(machineId);
    if (it != m_connections.end() && it->second->connected.load()) {
        return it->second->client->getMachinePosition();
    }
    
    return {0.0f, 0.0f, 0.0f}; // Default position
}

std::vector<float> CommunicationManager::GetWorkPosition(const std::string& machineId) const
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    auto it = m_connections.find(machineId);
    if (it != m_connections.end() && it->second->connected.load()) {
        return it->second->client->getWorkPosition();
    }
    
    return {0.0f, 0.0f, 0.0f}; // Default position
}

void CommunicationManager::DisconnectAll()
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    LOG_INFO("Disconnecting all machines...");
    
    for (auto& pair : m_connections) {
        LOG_INFO("Stopping connection for machine: " + pair.first);
        pair.second->client->stop();
        pair.second->connected = false;
    }
    
    m_connections.clear();
    LOG_INFO("All machines disconnected");
}

// Private callback methods (called from FluidNC client threads)

void CommunicationManager::OnConnect(const std::string& machineId)
{
    LOG_INFO("OnConnect begin for machine: " + machineId);
    
    bool shouldNotify = false;
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        auto it = m_connections.find(machineId);
        if (it != m_connections.end()) {
            if (!it->second->connected.load()) {
                it->second->connected = true;
                shouldNotify = true;
            }
        }
    }
    
    if (!shouldNotify) {
        LOG_INFO("OnConnect - Machine " + machineId + " already marked as connected, skipping notifications");
        return;
    }
    
    LOG_INFO("Machine connected: " + machineId);
    
    // Send initial status query to get machine info
    // Delay the initial query slightly to ensure connection is stable
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (SendCommand(machineId, "?")) {
        LOG_INFO("Sent initial status query to " + machineId);
    }
    
    // Store callbacks locally to prevent them changing during execution
    ConnectionStatusCallback statusCallback;
    MessageCallback msgCallback;
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        statusCallback = m_connectionStatusCallback;
        msgCallback = m_messageCallback;
    }
    
    // Execute callbacks outside the lock
    if (statusCallback) {
        UIQueue::getInstance().push([statusCallback, machineId]() {
            try {
                statusCallback(machineId, true);
            } catch (const std::exception& e) {
                LOG_ERROR("Exception in connection status callback: " + std::string(e.what()));
            }
        });
    }
    
    if (msgCallback) {
        UIQueue::getInstance().push([msgCallback, machineId]() {
            try {
                msgCallback(machineId, "Connected to machine: " + machineId, "INFO");
            } catch (const std::exception& e) {
                LOG_ERROR("Exception in message callback: " + std::string(e.what()));
            }
        });
    }
    
    LOG_INFO("OnConnect complete for machine: " + machineId);
}

void CommunicationManager::OnDisconnect(const std::string& machineId)
{
    LOG_INFO("OnDisconnect begin for machine: " + machineId);
    
    bool shouldNotify = false;
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        auto it = m_connections.find(machineId);
        if (it != m_connections.end()) {
            if (it->second->connected.load()) {
                it->second->connected = false;
                shouldNotify = true;
            }
        }
    }
    
    if (!shouldNotify) {
        LOG_INFO("OnDisconnect - Machine " + machineId + " already marked as disconnected, skipping notifications");
        return;
    }
    
    LOG_INFO("Machine disconnected: " + machineId);
    
    // Store callbacks locally to prevent them changing during execution
    ConnectionStatusCallback statusCallback;
    MessageCallback msgCallback;
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        statusCallback = m_connectionStatusCallback;
        msgCallback = m_messageCallback;
    }
    
    // Execute callbacks outside the lock
    if (statusCallback) {
        UIQueue::getInstance().push([statusCallback, machineId]() {
            try {
                statusCallback(machineId, false);
            } catch (const std::exception& e) {
                LOG_ERROR("Exception in connection status callback: " + std::string(e.what()));
            }
        });
    }
    
    if (msgCallback) {
        UIQueue::getInstance().push([msgCallback, machineId]() {
            try {
                msgCallback(machineId, "Disconnected from machine: " + machineId, "WARNING");
            } catch (const std::exception& e) {
                LOG_ERROR("Exception in message callback: " + std::string(e.what()));
            }
        });
    }
    
    LOG_INFO("OnDisconnect complete for machine: " + machineId);
}

void CommunicationManager::OnResponse(const std::string& machineId, const std::string& response)
{
    LOG_INFO("Response from " + machineId + ": " + response);
    
    // Notify GUI - callback must handle thread safety
    if (m_responseReceivedCallback) {
        m_responseReceivedCallback(machineId, response);
    }
}

void CommunicationManager::OnDROUpdate(const std::string& machineId, const std::vector<float>& mpos, const std::vector<float>& wpos)
{
    // This is called frequently, so only log periodically or not at all
    // LOG_DEBUG("DRO update for " + machineId);
    
    // Notify GUI - callback must handle thread safety
    if (m_droUpdateCallback) {
        m_droUpdateCallback(machineId, mpos, wpos);
    }
}
