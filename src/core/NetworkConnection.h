#pragma once

#ifdef _WIN32
    #ifndef UNICODE
    #define UNICODE
    #endif
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET socket_t;
#else
    #include <sys/socket.h>
    typedef int socket_t;
#endif

#include "NetworkManager.h"
#include <string>
#include <vector>

class NetworkConnection {
public:
    NetworkConnection(const std::string& ip, int port);
    ~NetworkConnection();

    bool connect(const ConnectionOptions& options);
    void disconnect();
    bool isConnected() const { return m_connected; }

    bool send(const std::string& data);
    std::string receive(size_t maxBytes = 4096);

    const std::string& getIP() const { return m_ip; }
    int getPort() const { return m_port; }

private:
    void configureKeepalive(const ConnectionOptions& options);

    std::string m_ip;
    int m_port;
    socket_t m_socket;
    bool m_connected;
};
