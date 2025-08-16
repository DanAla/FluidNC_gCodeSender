#pragma once

#include <string>
#include <vector>
#include <utility>
#include <mutex>
#include <map>
#include <memory>

// Forward declarations
class NetworkConnection;

// Connection options
struct ConnectionOptions {
    int connectTimeoutMs = 3000;  // Connection timeout in milliseconds
    bool keepAlive = true;        // Enable TCP keepalive
    int keepAliveIdleTime = 30;   // Seconds before first keepalive
    int keepAliveInterval = 10;   // Seconds between keepalives
    int keepAliveCount = 3;       // Number of keepalive probes
};

class NetworkManager {
public:
    static NetworkManager& getInstance() {
        static NetworkManager instance;
        return instance;
    }

    bool initialize();
    void cleanup();
    bool isInitialized() const { return m_initialized; }

    // Network operations
    bool testTcpPort(const std::string& ip, int port);
    bool sendPing(const std::string& ip, int& responseTime);
    std::vector<std::pair<std::string, std::string>> getNetworkAdapters();  // returns IP, Subnet pairs
    std::string resolveHostname(const std::string& ip);

    // Connection management
    std::shared_ptr<NetworkConnection> openConnection(const std::string& ip, int port, const ConnectionOptions& options = ConnectionOptions());
    bool closeConnection(const std::shared_ptr<NetworkConnection>& connection);
    size_t getActiveConnectionCount();
    void closeAllConnections();

private:
    NetworkManager() : m_initialized(false) {}
    ~NetworkManager() { cleanup(); }
    
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    void cleanupConnection(const std::string& connectionId);
    std::string generateConnectionId(const std::string& ip, int port);

    bool m_initialized;
    mutable std::mutex m_connectionsMutex;
    std::map<std::string, std::weak_ptr<NetworkConnection>> m_connections;
};
