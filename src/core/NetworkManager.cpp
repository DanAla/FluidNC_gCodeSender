#ifdef _WIN32
    #ifndef UNICODE
    #define UNICODE
    #endif
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #include <icmpapi.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "icmp.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

#include "NetworkManager.h"
#include "NetworkConnection.h"
#include "SimpleLogger.h"
#include "ErrorHandler.h"
#include <cstdint>
#include <sstream>
#include <random>

bool NetworkManager::initialize() {
    if (m_initialized) return true;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("Failed to initialize Winsock");
        return false;
    }
#endif

    m_initialized = true;
    return true;
}

void NetworkManager::cleanup() {
    if (!m_initialized) return;

#ifdef _WIN32
    WSACleanup();
#endif

    m_initialized = false;
}

bool NetworkManager::testTcpPort(const std::string& ip, int port) {
    if (!m_initialized) return false;

#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return false;

    // Set non-blocking mode
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    int result = connect(sock, (sockaddr*)&addr, sizeof(addr));

    bool success = false;
    if (result == 0) {
        success = true;
    } else if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(sock, &writeSet);

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 500000;  // 500ms timeout

            int selectResult = select(0, NULL, &writeSet, NULL, &timeout);
            if (selectResult > 0) {
                int optval;
                int optlen = sizeof(optval);
                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&optval, &optlen) == 0) {
                    success = (optval == 0);
                }
            }
        }
    }

    closesocket(sock);
    return success;
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    bool success = false;
    if (result == 0) {
        success = true;
    } else if (errno == EINPROGRESS) {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;

        int selectResult = select(sock + 1, NULL, &writeSet, NULL, &timeout);
        if (selectResult > 0) {
            int optval;
            socklen_t optlen = sizeof(optval);
            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &optval, &optlen) == 0) {
                success = (optval == 0);
            }
        }
    }

    close(sock);
    return success;
#endif
}

bool NetworkManager::sendPing(const std::string& ip, int& responseTime) {
    if (!m_initialized) return false;

#ifdef _WIN32
    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) return false;

    unsigned long ipaddr = inet_addr(ip.c_str());
    if (ipaddr == INADDR_NONE) {
        IcmpCloseHandle(hIcmp);
        return false;
    }

    char SendData[] = "Ping";
    DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
    LPVOID ReplyBuffer = malloc(ReplySize);

    if (!ReplyBuffer) {
        IcmpCloseHandle(hIcmp);
        return false;
    }

    DWORD dwRetVal = IcmpSendEcho(hIcmp, ipaddr, SendData, sizeof(SendData),
                                 NULL, ReplyBuffer, ReplySize, 100);  // 100ms timeout

    bool success = false;
    if (dwRetVal != 0) {
        PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
        if (pEchoReply->Status == IP_SUCCESS) {
            responseTime = pEchoReply->RoundTripTime;
            success = true;
        }
    }

    free(ReplyBuffer);
    IcmpCloseHandle(hIcmp);
    return success;
#else
    responseTime = -1;
    return false;  // Not implemented for Unix yet
#endif
}

std::vector<std::pair<std::string, std::string>> NetworkManager::getNetworkAdapters() {
    std::vector<std::pair<std::string, std::string>> adapters;
    if (!m_initialized) return adapters;

#ifdef _WIN32
    ULONG bufferLength = 0;
    GetAdaptersInfo(nullptr, &bufferLength);

    if (bufferLength > 0) {
        PIP_ADAPTER_INFO adapterInfo = (PIP_ADAPTER_INFO)malloc(bufferLength);
        if (adapterInfo && GetAdaptersInfo(adapterInfo, &bufferLength) == ERROR_SUCCESS) {
            PIP_ADAPTER_INFO adapter = adapterInfo;

            while (adapter) {
                if (adapter->Type == MIB_IF_TYPE_ETHERNET || adapter->Type == IF_TYPE_IEEE80211) {
                    std::string ipStr = adapter->IpAddressList.IpAddress.String;
                    std::string maskStr = adapter->IpAddressList.IpMask.String;

                    if (ipStr != "0.0.0.0" && ipStr != "127.0.0.1" && !ipStr.empty() &&
                        ipStr.find("169.254.") != 0) {

                        struct in_addr ip, mask, network;
                        inet_pton(AF_INET, ipStr.c_str(), &ip);
                        inet_pton(AF_INET, maskStr.c_str(), &mask);

                        #ifdef _WIN32
                        network.S_un.S_addr = ip.S_un.S_addr & mask.S_un.S_addr;
                        #else
                        network.s_addr = ip.s_addr & mask.s_addr;
                        #endif

                        int cidr = 0;
                        uint32_t maskVal = ntohl(mask.s_addr);
                        while (maskVal & 0x80000000) {
                            cidr++;
                            maskVal <<= 1;
                        }

                        std::string networkStr = inet_ntoa(network);
                        std::string subnet = networkStr + "/" + std::to_string(cidr);

                        adapters.push_back(std::make_pair(ipStr, subnet));
                    }
                }
                adapter = adapter->Next;
            }
        }

        if (adapterInfo) {
            free(adapterInfo);
        }
    }
#endif

    return adapters;
}

std::string NetworkManager::resolveHostname(const std::string& ip) {
    if (!m_initialized) return "";

    struct sockaddr_in sa;
    char host[NI_MAXHOST];

    sa.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);

    if (getnameinfo((struct sockaddr*)&sa, sizeof(sa), host, sizeof(host), NULL, 0, NI_NAMEREQD) == 0) {
        return std::string(host);
    }

    return "";
}

std::shared_ptr<NetworkConnection> NetworkManager::openConnection(const std::string& ip, int port, const ConnectionOptions& options) {
    if (!m_initialized) {
        LOG_ERROR("NetworkManager not initialized");
        return nullptr;
    }

    std::string connectionId = generateConnectionId(ip, port);
    
    // Check if connection already exists
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        auto it = m_connections.find(connectionId);
        if (it != m_connections.end()) {
            if (auto existingConn = it->second.lock()) {
                if (existingConn->isConnected()) {
                    return existingConn;
                }
            }
            // Remove expired connection
            m_connections.erase(it);
        }
    }

    // Create new connection
    auto connection = std::make_shared<NetworkConnection>(ip, port);
    if (!connection->connect(options)) {
        LOG_ERROR("Failed to connect to " + ip + ":" + std::to_string(port));
        return nullptr;
    }

    // Store the connection
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        m_connections[connectionId] = connection;
    }

    return connection;
}

bool NetworkManager::closeConnection(const std::shared_ptr<NetworkConnection>& connection) {
    if (!connection) return false;

    std::string connectionId = generateConnectionId(connection->getIP(), connection->getPort());
    
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        auto it = m_connections.find(connectionId);
        if (it != m_connections.end()) {
            if (auto conn = it->second.lock()) {
                if (conn == connection) {
                    conn->disconnect();
                    m_connections.erase(it);
                    return true;
                }
            }
            m_connections.erase(it);
        }
    }

    return false;
}

size_t NetworkManager::getActiveConnectionCount() {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    size_t count = 0;
    
    // Clean up expired connections while counting
    for (auto it = m_connections.begin(); it != m_connections.end();) {
        if (auto conn = it->second.lock()) {
            if (conn->isConnected()) {
                count++;
                ++it;
                continue;
            }
        }
        it = m_connections.erase(it);
    }
    
    return count;
}

void NetworkManager::closeAllConnections() {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    for (auto it = m_connections.begin(); it != m_connections.end();) {
        if (auto conn = it->second.lock()) {
            conn->disconnect();
        }
        it = m_connections.erase(it);
    }
}

void NetworkManager::cleanupConnection(const std::string& connectionId) {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    m_connections.erase(connectionId);
}

std::string NetworkManager::generateConnectionId(const std::string& ip, int port) {
    return ip + ":" + std::to_string(port);
}
