/**
 * core/CommunicationManager.cpp
 * Implementation of real communication between GUI and FluidNC machines
 */

#include "CommunicationManager.h"
#include "SimpleLogger.h"

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
    
    // Check if already connected
    auto it = m_connections.find(machineId);
    if (it != m_connections.end() && it->second->connected.load()) {
        LOG_ERROR("Machine " + machineId + " is already connected");
        return true;
    }
    
    // Create new connection
    try {
        auto connectionInfo = std::make_unique<ConnectionInfo>();
        connectionInfo->machineId = machineId;
        connectionInfo->host = host;
        connectionInfo->port = port;
        connectionInfo->connected = false;
        
        // Create FluidNC client with DRO callback
        connectionInfo->client = std::make_unique<FluidNCClient>(
            host, port,
            [this, machineId](const std::vector<float>& mpos, const std::vector<float>& wpos) {
                OnDROUpdate(machineId, mpos, wpos);
            }
        );
        
        // Set connection callbacks
        connectionInfo->client->setOnConnectCallback([this, machineId]() {
            OnConnect(machineId);
        });
        
        connectionInfo->client->setOnDisconnectCallback([this, machineId]() {
            OnDisconnect(machineId);
        });
        
        // Set response callback
        connectionInfo->client->setResponseCallback([this, machineId](const std::string& response) {
            OnResponse(machineId, response);
        });
        
        // Start the client (this will attempt connection)
        connectionInfo->client->start();
        
        // Store the connection
        m_connections[machineId] = std::move(connectionInfo);
        
        LOG_INFO("Started connection attempt for machine: " + machineId + " (" + host + ":" + std::to_string(port) + ")");
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to create connection for machine " + machineId + ": " + e.what());
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
            // Call callback directly - GUI code must handle thread safety
            m_commandSentCallback(machineId, command);
        }
        
        // Send the command to the machine
        it->second->client->sendGCodeLine(command);
        
        LOG_INFO("Sent command to " + machineId + ": " + command);
        return true;
    } else {
        LOG_ERROR("Cannot send command to disconnected machine: " + machineId);
        
        // Notify GUI of error
        if (m_messageCallback) {
            // Call callback directly - GUI code must handle thread safety
            m_messageCallback(machineId, "Cannot send command - machine not connected", "ERROR");
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
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        auto it = m_connections.find(machineId);
        if (it != m_connections.end()) {
            it->second->connected = true;
        }
    }
    
    LOG_INFO("Machine connected: " + machineId);
    
    // Send initial status query to get machine info
    if (SendCommand(machineId, "?")) {
        LOG_INFO("Sent initial status query to " + machineId);
    }
    
    // Notify GUI - callbacks must handle thread safety
    if (m_connectionStatusCallback) {
        m_connectionStatusCallback(machineId, true);
    }
    
    if (m_messageCallback) {
        m_messageCallback(machineId, "Connected to machine: " + machineId, "INFO");
    }
}

void CommunicationManager::OnDisconnect(const std::string& machineId)
{
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        auto it = m_connections.find(machineId);
        if (it != m_connections.end()) {
            it->second->connected = false;
        }
    }
    
    LOG_INFO("Machine disconnected: " + machineId);
    
    // Notify GUI - callbacks must handle thread safety
    if (m_connectionStatusCallback) {
        m_connectionStatusCallback(machineId, false);
    }
    
    if (m_messageCallback) {
        m_messageCallback(machineId, "Disconnected from machine: " + machineId, "WARNING");
    }
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
