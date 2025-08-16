/**
 * core/FluidNCClient.h
 * Multi-protocol CNC machine client supporting Telnet, USB, and UART
 * Provides real-time DRO, streaming G-Code with ack, manual commands
 */

#pragma once

#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#endif

#include "StateManager.h"
#include <string>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <functional>
#include <vector>
#include <condition_variable>
#include <memory>

#include "NetworkManager.h"

class FluidNCClient
{
public:
    // Callback types
    using DROCallback = std::function<void(const std::vector<float>& mpos, const std::vector<float>& wpos)>;
    using ConnectionCallback = std::function<void()>;
    using ResponseCallback = std::function<void(const std::string& response)>;
    
    FluidNCClient(const std::string& host, int port, DROCallback droCallback = nullptr);
    ~FluidNCClient();
    
    // Connection management
    void start();
    void stop();
    bool isConnected() const { return m_connected.load(); }
    
    // G-code sending
    void sendGCodeLine(const std::string& line);
    
    // Callbacks (GUI should wrap these with appropriate thread dispatching)
    void setOnConnectCallback(ConnectionCallback callback) { m_onConnect = callback; }
    void setOnDisconnectCallback(ConnectionCallback callback) { m_onDisconnect = callback; }
    void setResponseCallback(ResponseCallback callback) { m_onResponse = callback; }
    
    // Settings
    void setAutoReconnect(bool enable) { m_autoReconnect = enable; }
    bool getAutoReconnect() const { return m_autoReconnect.load(); }
    
    // Current position access (thread-safe)
    std::vector<float> getMachinePosition() const;
    std::vector<float> getWorkPosition() const;

private:
    void rxLoop();      // Receive thread
    void txLoop();      // Transmit thread
    void connect();     // Connection attempt
    void handleLine(const std::string& line);  // Parse incoming data
    void closeSocket();
    
    // Network
    std::string m_host;
    int m_port;
    std::shared_ptr<NetworkConnection> m_connection;
    std::atomic<bool> m_connected;
    std::atomic<bool> m_autoReconnect;

    // Threading
    std::atomic<bool> m_running;
    std::thread m_rxThread;
    std::thread m_txThread;

    // Command queue
    std::queue<std::string> m_txQueue;
    std::mutex m_txMutex;
    std::condition_variable m_txCondition;

    // DRO data (thread-safe)
    mutable std::mutex m_droMutex;
    std::vector<float> m_machinePos;
    std::vector<float> m_workPos;

    // Callbacks
    DROCallback m_droCallback;
    ConnectionCallback m_onConnect;
    ConnectionCallback m_onDisconnect;
    ResponseCallback m_onResponse;

    // NetworkManager reference
    NetworkManager& m_networkManager;
};
