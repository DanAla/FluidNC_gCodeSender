/**
 * core/ConnectionManager.cpp
 * Multi-protocol connection manager implementation (basic version)
 */

#include "ConnectionManager.h"
#include <iostream>

ConnectionManager::ConnectionManager()
    : m_stateManager(StateManager::getInstance())
{
    // Load existing machine configurations
    auto machines = m_stateManager.getMachines();
    for (const auto& machine : machines) {
        m_machines[machine.id] = machine;
    }
}

ConnectionManager::~ConnectionManager()
{
    disconnectAll();
}

bool ConnectionManager::addMachine(const MachineConfig& config)
{
    std::lock_guard<std::recursive_mutex> lock(m_machinesMutex);
    
    m_machines[config.id] = config;
    m_stateManager.addMachine(config);
    
    return true;
}

bool ConnectionManager::removeMachine(const std::string& machineId)
{
    std::lock_guard<std::recursive_mutex> lock(m_machinesMutex);
    
    // Disconnect if connected
    disconnectMachine(machineId);
    
    // Remove from memory
    m_machines.erase(machineId);
    m_connections.erase(machineId);
    m_statuses.erase(machineId);
    
    // Remove from persistent storage
    m_stateManager.removeMachine(machineId);
    
    return true;
}

bool ConnectionManager::updateMachine(const MachineConfig& config)
{
    std::lock_guard<std::recursive_mutex> lock(m_machinesMutex);
    
    m_machines[config.id] = config;
    m_stateManager.updateMachine(config.id, config);
    
    return true;
}

std::vector<std::string> ConnectionManager::getMachineIds() const
{
    std::lock_guard<std::recursive_mutex> lock(m_machinesMutex);
    
    std::vector<std::string> ids;
    for (const auto& pair : m_machines) {
        ids.push_back(pair.first);
    }
    return ids;
}

MachineConfig ConnectionManager::getMachineConfig(const std::string& machineId) const
{
    std::lock_guard<std::recursive_mutex> lock(m_machinesMutex);
    
    auto it = m_machines.find(machineId);
    if (it != m_machines.end()) {
        return it->second;
    }
    
    return MachineConfig(); // Return empty config if not found
}

bool ConnectionManager::connectMachine(const std::string& machineId)
{
    // TODO: Implement actual connection logic
    // For now, just update status
    MachineStatus status;
    status.machineId = machineId;
    status.status = ConnectionStatus::Connected;
    status.currentState = "Idle";
    
    updateMachineStatus(machineId, status);
    
    return true;
}

bool ConnectionManager::disconnectMachine(const std::string& machineId)
{
    // TODO: Implement actual disconnection logic
    MachineStatus status;
    status.machineId = machineId;
    status.status = ConnectionStatus::Disconnected;
    status.currentState = "Offline";
    
    updateMachineStatus(machineId, status);
    
    return true;
}

void ConnectionManager::disconnectAll()
{
    std::lock_guard<std::recursive_mutex> lock(m_machinesMutex);
    
    for (const auto& pair : m_machines) {
        disconnectMachine(pair.first);
    }
}

bool ConnectionManager::isConnected(const std::string& machineId) const
{
    std::lock_guard<std::recursive_mutex> lock(m_machinesMutex);
    
    auto it = m_statuses.find(machineId);
    if (it != m_statuses.end()) {
        return it->second.status == ConnectionStatus::Connected;
    }
    
    return false;
}

ConnectionStatus ConnectionManager::getConnectionStatus(const std::string& machineId) const
{
    std::lock_guard<std::recursive_mutex> lock(m_machinesMutex);
    
    auto it = m_statuses.find(machineId);
    if (it != m_statuses.end()) {
        return it->second.status;
    }
    
    return ConnectionStatus::Disconnected;
}

void ConnectionManager::setActiveMachine(const std::string& machineId)
{
    std::lock_guard<std::recursive_mutex> lock(m_machinesMutex);
    
    if (m_machines.find(machineId) != m_machines.end()) {
        m_activeMachine = machineId;
        m_stateManager.setActiveMachine(machineId);
    }
}

std::string ConnectionManager::getActiveMachine() const
{
    std::lock_guard<std::recursive_mutex> lock(m_machinesMutex);
    return m_activeMachine;
}

bool ConnectionManager::sendCommand(const std::string& machineId, const std::string& command)
{
    // TODO: Implement actual command sending
    std::cout << "Sending to " << machineId << ": " << command << std::endl;
    return true;
}

bool ConnectionManager::sendCommandToActive(const std::string& command)
{
    std::string active = getActiveMachine();
    if (!active.empty()) {
        return sendCommand(active, command);
    }
    return false;
}

bool ConnectionManager::sendFile(const std::string& machineId, const std::vector<std::string>& gcode)
{
    // TODO: Implement file sending
    for (const auto& line : gcode) {
        if (!sendCommand(machineId, line)) {
            return false;
        }
    }
    return true;
}

void ConnectionManager::emergencyStop(const std::string& machineId)
{
    if (machineId.empty()) {
        // Stop all machines
        for (const auto& pair : m_machines) {
            sendCommand(pair.first, "!");  // FluidNC emergency stop
        }
    } else {
        sendCommand(machineId, "!");
    }
}

MachineStatus ConnectionManager::getMachineStatus(const std::string& machineId) const
{
    std::lock_guard<std::recursive_mutex> lock(m_machinesMutex);
    
    auto it = m_statuses.find(machineId);
    if (it != m_statuses.end()) {
        return it->second;
    }
    
    // Return default status
    MachineStatus status;
    status.machineId = machineId;
    status.status = ConnectionStatus::Disconnected;
    return status;
}

std::vector<MachineStatus> ConnectionManager::getAllStatuses() const
{
    std::lock_guard<std::recursive_mutex> lock(m_machinesMutex);
    
    std::vector<MachineStatus> statuses;
    for (const auto& pair : m_statuses) {
        statuses.push_back(pair.second);
    }
    return statuses;
}

void ConnectionManager::enableAutoConnect(bool enable)
{
    m_autoConnect = enable;
}

std::unique_ptr<IConnection> ConnectionManager::createConnection(const MachineConfig& config)
{
    // TODO: Create actual connection objects based on type
    // For now, return nullptr
    return nullptr;
}

void ConnectionManager::connectionThread(const std::string& machineId)
{
    // TODO: Implement connection thread
}

void ConnectionManager::updateMachineStatus(const std::string& machineId, const MachineStatus& status)
{
    {
        std::lock_guard<std::recursive_mutex> lock(m_machinesMutex);
        m_statuses[machineId] = status;
    }
    
    // Call callback if set
    if (m_statusCallback) {
        m_statusCallback(machineId, status);
    }
}
