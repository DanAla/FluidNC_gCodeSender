/**
 * core/FluidNCClient.cpp
 * Implementation of asynchronous FluidNC telnet client
 */

#include "FluidNCClient.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

FluidNCClient::FluidNCClient(const std::string& host, int port, DROCallback droCallback)
    : m_host(host), m_port(port), m_socket(INVALID_SOCKET),
      m_connected(false), m_autoReconnect(false), m_running(false),
      m_machinePos(3, 0.0f), m_workPos(3, 0.0f),
      m_droCallback(droCallback)
{
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
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
    if (m_running.load()) {
        return;
    }
    
    m_running = true;
    m_rxThread = std::thread(&FluidNCClient::rxLoop, this);
    m_txThread = std::thread(&FluidNCClient::txLoop, this);
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
    char buffer[4096];
    std::string lineBuffer;
    
    while (m_running.load()) {
        if (!m_connected.load()) {
            connect();
            if (!m_connected.load()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
        }
        
        // Receive data
#ifdef _WIN32
        int bytesReceived = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
#else
        ssize_t bytesReceived = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
#endif
        
        if (bytesReceived <= 0) {
            // Connection lost
            m_connected = false;
            closeSocket();
            if (m_onDisconnect) {
                m_onDisconnect();
            }
            continue;
        }
        
        buffer[bytesReceived] = '\0';
        lineBuffer += buffer;
        
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
}

void FluidNCClient::txLoop()
{
    while (m_running.load()) {
        std::unique_lock<std::mutex> lock(m_txMutex);
        
        // Wait for commands or stop signal
        m_txCondition.wait(lock, [this] {
            return !m_txQueue.empty() || !m_running.load();
        });
        
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
        if (m_connected.load() && m_socket != INVALID_SOCKET) {
            std::string commandWithCRLF = command + "\r\n";
#ifdef _WIN32
            int result = send(m_socket, commandWithCRLF.c_str(), 
                             static_cast<int>(commandWithCRLF.length()), 0);
#else
            ssize_t result = send(m_socket, commandWithCRLF.c_str(), 
                                 commandWithCRLF.length(), 0);
#endif
            if (result == SOCKET_ERROR) {
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
    while (m_running.load() && !m_connected.load()) {
        closeSocket();  // Ensure clean state
        
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket == INVALID_SOCKET) {
            if (!m_autoReconnect.load()) break;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        
        sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(static_cast<uint16_t>(m_port));
        
#ifdef _WIN32
        serverAddr.sin_addr.s_addr = inet_addr(m_host.c_str());
        if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
#else
        if (inet_aton(m_host.c_str(), &serverAddr.sin_addr) == 0) {
#endif
            // Host is not an IP address, try hostname resolution
            struct addrinfo hints, *result;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET; // IPv4
            hints.ai_socktype = SOCK_STREAM;
            
            int status = getaddrinfo(m_host.c_str(), nullptr, &hints, &result);
            if (status != 0 || result == nullptr) {
                std::cerr << "Failed to resolve hostname: " << m_host << std::endl;
                closeSocket();
                if (!m_autoReconnect.load()) break;
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }
            
            // Use the first result
            struct sockaddr_in* addr_in = reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
            serverAddr.sin_addr = addr_in->sin_addr;
            
            freeaddrinfo(result);
        }
        
        if (::connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            closeSocket();
            if (!m_autoReconnect.load()) break;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        
        // Connection successful
        m_connected = true;
        if (m_onConnect) {
            m_onConnect();
        }
        break;
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
    if (m_socket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
    }
}
