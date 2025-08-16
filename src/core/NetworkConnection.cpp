#include "NetworkConnection.h"
#include "SimpleLogger.h"

#ifdef _WIN32
    #include <mstcpip.h>  // For TCP keepalive
#else
    #include <netinet/in.h>
    #include <netinet/tcp.h>  // For TCP keepalive
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

NetworkConnection::NetworkConnection(const std::string& ip, int port)
    : m_ip(ip), m_port(port), m_socket(INVALID_SOCKET), m_connected(false) {}

NetworkConnection::~NetworkConnection() {
    disconnect();
}

bool NetworkConnection::connect(const ConnectionOptions& options) {
    if (m_connected) return true;

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        LOG_ERROR("Failed to create socket");
        return false;
    }

    // Set non-blocking mode for timeout support
    #ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(m_socket, FIONBIO, &mode);
    #else
        int flags = fcntl(m_socket, F_GETFL, 0);
        fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
    #endif

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    if (inet_pton(AF_INET, m_ip.c_str(), &addr.sin_addr) <= 0) {
        LOG_ERROR("Invalid IP address: " + m_ip);
        disconnect();
        return false;
    }

    // Attempt connection
    int result = ::connect(m_socket, (sockaddr*)&addr, sizeof(addr));
    if (result == SOCKET_ERROR) {
        #ifdef _WIN32
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                disconnect();
                return false;
            }
        #else
            if (errno != EINPROGRESS) {
                disconnect();
                return false;
            }
        #endif

        // Wait for connection with timeout
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_socket, &writeSet);

        struct timeval timeout;
        timeout.tv_sec = options.connectTimeoutMs / 1000;
        timeout.tv_usec = (options.connectTimeoutMs % 1000) * 1000;

        result = select(m_socket + 1, NULL, &writeSet, NULL, &timeout);
        if (result <= 0) {
            disconnect();
            return false;
        }

        // Verify connection success
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0 || error != 0) {
            disconnect();
            return false;
        }
    }

    // Connection successful - configure keepalive if requested
    if (options.keepAlive) {
        configureKeepalive(options);
    }

    // Return to blocking mode
    #ifdef _WIN32
        mode = 0;
        ioctlsocket(m_socket, FIONBIO, &mode);
    #else
        flags = fcntl(m_socket, F_GETFL, 0);
        fcntl(m_socket, F_SETFL, flags & ~O_NONBLOCK);
    #endif

    m_connected = true;
    return true;
}

void NetworkConnection::disconnect() {
    if (m_socket != INVALID_SOCKET) {
        #ifdef _WIN32
            closesocket(m_socket);
        #else
            close(m_socket);
        #endif
        m_socket = INVALID_SOCKET;
    }
    m_connected = false;
}

bool NetworkConnection::send(const std::string& data) {
    if (!m_connected) return false;
    return ::send(m_socket, data.c_str(), data.length(), 0) != SOCKET_ERROR;
}

std::string NetworkConnection::receive(size_t maxBytes) {
    if (!m_connected) return "";
    
    std::vector<char> buffer(maxBytes);
    int bytesRead = recv(m_socket, buffer.data(), buffer.size(), 0);
    
    if (bytesRead <= 0) {
        disconnect();
        return "";
    }
    
    return std::string(buffer.data(), bytesRead);
}

void NetworkConnection::configureKeepalive(const ConnectionOptions& options) {
    #ifdef _WIN32
        DWORD dwBytesRet = 0;
        struct tcp_keepalive keepalive_vals = {
            1,                                          // Enable keepalive
            static_cast<ULONG>(options.keepAliveIdleTime * 1000),    // Idle time in ms
            static_cast<ULONG>(options.keepAliveInterval * 1000)     // Interval in ms
        };
        WSAIoctl(m_socket, SIO_KEEPALIVE_VALS, &keepalive_vals,
                 sizeof(keepalive_vals), NULL, 0, &dwBytesRet, NULL, NULL);
    #else
        int keepalive = 1;
        setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
        setsockopt(m_socket, IPPROTO_TCP, TCP_KEEPIDLE, &options.keepAliveIdleTime, sizeof(int));
        setsockopt(m_socket, IPPROTO_TCP, TCP_KEEPINTVL, &options.keepAliveInterval, sizeof(int));
        setsockopt(m_socket, IPPROTO_TCP, TCP_KEEPCNT, &options.keepAliveCount, sizeof(int));
    #endif
}
