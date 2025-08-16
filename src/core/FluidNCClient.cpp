/**
 * core/FluidNCClient.cpp
 * Implementation of asynchronous FluidNC telnet client
 */

#ifdef _WIN32
    #ifndef UNICODE
    #define UNICODE
    #endif
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
#endif

#include "FluidNCClient.h"
#include "NetworkConnection.h"
#include "ErrorHandler.h"
#include "StringUtils.h"
#include "SimpleLogger.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <algorithm>

FluidNCClient::FluidNCClient(const std::string& host, int port, DROCallback droCallback)
    : m_host(host), m_port(port),
      m_connected(false), m_autoReconnect(false), m_running(false),
      m_machinePos(3, 0.0f), m_workPos(3, 0.0f),
      m_droCallback(droCallback),
      m_networkManager(NetworkManager::getInstance())
{
    if (!m_networkManager.isInitialized()) {
        m_networkManager.initialize();
    }
}

FluidNCClient::~FluidNCClient()
{
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void FluidNCClient::start()
{
    LOG_INFO("FluidNCClient::start() - Starting client for " + m_host + ":" + std::to_string(m_port));
    
    if (m_running.load()) {
        LOG_INFO("FluidNCClient::start() - Client already running");
        return;
    }
    
    try {
        m_running = true;
        LOG_INFO("FluidNCClient::start() - Starting rx/tx threads");
        m_rxThread = std::thread(&FluidNCClient::rxLoop, this);
        m_txThread = std::thread(&FluidNCClient::txLoop, this);
        LOG_INFO("FluidNCClient::start() - Threads started successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("FluidNCClient::start() - Failed to start threads: " + std::string(e.what()));
        m_running = false;
        throw;
    }
}

void FluidNCClient::stop()
{
    m_running = false;
    m_autoReconnect = false;
    
    // Wake up tx thread
    m_txCondition.notify_all();
    
    closeSocket();
    
    // Join threads
    if (m_rxThread.joinable()) {
        m_rxThread.join();
    }
    if (m_txThread.joinable()) {
        m_txThread.join();
    }
}

void FluidNCClient::sendGCodeLine(const std::string& line)
{
    if (line.empty()) return;
    
    {
        std::lock_guard<std::mutex> lock(m_txMutex);
        m_txQueue.push(line);
    }
    m_txCondition.notify_one();
}

std::vector<float> FluidNCClient::getMachinePosition() const
{
    std::lock_guard<std::mutex> lock(m_droMutex);
    return m_machinePos;
}

std::vector<float> FluidNCClient::getWorkPosition() const
{
    std::lock_guard<std::mutex> lock(m_droMutex);
    return m_workPos;
}

void FluidNCClient::rxLoop()
{
    LOG_INFO("FluidNCClient::rxLoop() - Starting receive loop");
    std::string lineBuffer;
    
    try {
        while (m_running.load()) {
            if (!m_connected.load()) {
                LOG_INFO("FluidNCClient::rxLoop() - Not connected, attempting connection");
                try {
                    // Initial delay before connection attempt to prevent rapid reconnection
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    
                    connect();
                    if (!m_connected.load()) {
                        LOG_INFO("FluidNCClient::rxLoop() - Connection attempt failed, waiting before retry");
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        continue;
                    }
                    LOG_INFO("FluidNCClient::rxLoop() - Connection successful");
                    
                    // Add delay after successful connection before proceeding
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                } catch (const std::exception& e) {
                    LOG_ERROR("FluidNCClient::rxLoop() - Connection attempt failed with error: " + std::string(e.what()));
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }
            }

            // Receive data
            std::string data = m_connection->receive();
            if (data.empty()) {
                if (!m_connection->isConnected()) {
                    // Connection lost
                    LOG_ERROR("FluidNCClient::rxLoop() - Connection lost");
                    m_connected = false;
                    closeSocket();
                    if (m_onDisconnect) {
                        LOG_INFO("FluidNCClient::rxLoop() - Notifying disconnect handlers");
                        m_onDisconnect();
                    }
                    continue;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            LOG_INFO("FluidNCClient::rxLoop() - Received " + std::to_string(data.length()) + " bytes");
            
            lineBuffer += data;
            
            // Process complete lines
            size_t pos = 0;
            while ((pos = lineBuffer.find('\n')) != std::string::npos) {
                std::string line = lineBuffer.substr(0, pos);
                lineBuffer.erase(0, pos + 1);
                
                // Remove trailing \r if present
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                
                if (!line.empty()) {
                    handleLine(line);
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("FluidNCClient::rxLoop() - Unhandled exception: " + std::string(e.what()));
        throw;
    }
}

void FluidNCClient::txLoop()
{
    LOG_INFO("FluidNCClient::txLoop() - Starting transmit loop");
    while (m_running.load()) {
        std::unique_lock<std::mutex> lock(m_txMutex);
        
        // Wait for commands or stop signal
        LOG_INFO("FluidNCClient::txLoop() - Waiting for commands");
        m_txCondition.wait(lock, [this] {
            return !m_txQueue.empty() || !m_running.load();
        });
        
        LOG_INFO("FluidNCClient::txLoop() - Woke up, checking conditions");
        
        if (!m_running.load()) {
            break;
        }
        
        if (m_txQueue.empty()) {
            continue;
        }
        
        std::string command = m_txQueue.front();
        m_txQueue.pop();
        lock.unlock();
        
        // Send command if connected
        if (m_connected.load() && m_connection && m_connection->isConnected()) {
            std::string commandWithCRLF = command + "\r\n";
            if (!m_connection->send(commandWithCRLF)) {
                // Re-queue command and mark as disconnected
                {
                    std::lock_guard<std::mutex> requeueLock(m_txMutex);
                    // Put command back at front of queue
                    std::queue<std::string> tempQueue;
                    tempQueue.push(command);
                    while (!m_txQueue.empty()) {
                        tempQueue.push(m_txQueue.front());
                        m_txQueue.pop();
                    }
                    m_txQueue = std::move(tempQueue);
                }
                m_connected = false;
                closeSocket();
            }
        } else {
            // Re-queue command for later
            {
                std::lock_guard<std::mutex> requeueLock(m_txMutex);
                std::queue<std::string> tempQueue;
                tempQueue.push(command);
                while (!m_txQueue.empty()) {
                    tempQueue.push(m_txQueue.front());
                    m_txQueue.pop();
                }
                m_txQueue = std::move(tempQueue);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

void FluidNCClient::connect()
{
    LOG_INFO("FluidNCClient::connect() - Attempting connection to " + m_host + ":" + std::to_string(m_port));

    while (m_running.load() && !m_connected.load()) {
        LOG_INFO("FluidNCClient::connect() - Closing previous connection");
        if (m_connection) {
            m_networkManager.closeConnection(m_connection);
            m_connection = nullptr;
        }

        try {
            // Create connection options
            ConnectionOptions options;
            options.connectTimeoutMs = 5000;  // 5 seconds
            options.keepAlive = true;
            options.keepAliveIdleTime = 5;    // Start keepalive after 5 seconds
            options.keepAliveInterval = 2;    // Send keepalive probes every 2 seconds
            options.keepAliveCount = 3;       // Give up after 3 failed probes

            // Open connection
            m_connection = m_networkManager.openConnection(m_host, m_port, options);
            if (!m_connection || !m_connection->isConnected()) {
                m_connected = false;
                LOG_ERROR("FluidNCClient::connect() - Connection attempt failed");
                if (!m_autoReconnect.load()) {
                    throw std::runtime_error("Failed to connect to " + m_host + ":" + std::to_string(m_port));
                }
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }

            LOG_INFO("FluidNCClient::connect() - Connection successful");
            m_connected = true;
            if (m_onConnect) {
                m_onConnect();
            }
            break;

        } catch (const std::exception& e) {
            LOG_ERROR("FluidNCClient::connect() - Connection error: " + std::string(e.what()));
            ErrorHandler::Instance().ReportWarning(
                "Connection Error",
                "Failed to connect to " + m_host + ":" + std::to_string(m_port),
                std::string(e.what()));
                
            if (!m_autoReconnect.load()) {
                throw;
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
    }
}

void FluidNCClient::handleLine(const std::string& line)
{
    // Forward all responses to the communication manager
    if (m_onResponse) {
        m_onResponse(line);
    }
    
    // Parse FluidNC status messages like <Idle|MPos:0.000,0.000,0.000|WPos:0.000,0.000,0.000|F:0>
    if (line.length() >= 2 && line[0] == '<' && line.back() == '>') {
        std::string content = line.substr(1, line.length() - 2);
        std::stringstream ss(content);
        std::string part;
        
        bool mposUpdated = false, wposUpdated = false;
        std::vector<float> newMPos, newWPos;
        
        while (std::getline(ss, part, '|')) {
            if (part.substr(0, 5) == "MPos:") {
                std::string coords = part.substr(5);
                std::stringstream coordStream(coords);
                std::string coord;
                newMPos.clear();
                
                while (std::getline(coordStream, coord, ',')) {
                    try {
                        newMPos.push_back(std::stof(coord));
                    } catch (...) {
                        // Ignore parse errors
                    }
                }
                mposUpdated = !newMPos.empty();
            }
            else if (part.substr(0, 5) == "WPos:") {
                std::string coords = part.substr(5);
                std::stringstream coordStream(coords);
                std::string coord;
                newWPos.clear();
                
                while (std::getline(coordStream, coord, ',')) {
                    try {
                        newWPos.push_back(std::stof(coord));
                    } catch (...) {
                        // Ignore parse errors
                    }
                }
                wposUpdated = !newWPos.empty();
            }
        }
        
        // Update stored positions and call callback
        if (mposUpdated || wposUpdated) {
            {
                std::lock_guard<std::mutex> lock(m_droMutex);
                if (mposUpdated) {
                    m_machinePos = newMPos;
                }
                if (wposUpdated) {
                    m_workPos = newWPos;
                }
            }
            
            if (m_droCallback) {
                m_droCallback(m_machinePos, m_workPos);
            }
        }
    }
}

void FluidNCClient::closeSocket()
{
    LOG_INFO("FluidNCClient::closeSocket() - Closing connection if open");
    if (m_connection) {
        LOG_INFO("FluidNCClient::closeSocket() - Connection is open, closing it");
        m_networkManager.closeConnection(m_connection);
        m_connection = nullptr;
    }
}
