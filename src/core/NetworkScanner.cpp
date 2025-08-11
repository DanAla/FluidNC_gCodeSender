/**
 * core/NetworkScanner.cpp
 * Cross-platform network scanner implementation
 */

#include "NetworkScanner.h"
#include "SimpleLogger.h"
#include "MacVendorLookup.h"
#include <wx/log.h>
#include <wx/utils.h>
#include <algorithm>
#include <sstream>
#include <regex>

// Platform-specific includes
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #include <icmpapi.h>
    #include <ipifcons.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "icmp.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <ifaddrs.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

NetworkScanner::NetworkScanner()
    : wxThread(wxTHREAD_JOINABLE),
      m_isScanning(false),
      m_stopRequested(false)
{
}

NetworkScanner::~NetworkScanner()
{
    if (IsRunning()) {
        StopScan();
        Wait();
    }
}

void NetworkScanner::StartScan(const std::string& subnet)
{
    if (m_isScanning) return;
    
    {
        wxCriticalSectionLocker lock(m_criticalSection);
        m_isScanning = true;
        m_stopRequested = false;
        m_currentSubnet = subnet.empty() ? GetLocalSubnet() : subnet;
        m_devices.clear();
    }
    
    // Log scan start
    LOG_INFO("=== Network Scan Started ===");
    LOG_INFO("Scanning subnet: " + m_currentSubnet);
    LOG_INFO("Platform: " + GetPlatform());
    
    // Start the scanning thread
    if (Create() == wxTHREAD_NO_ERROR) {
        Run();
    }
}

void NetworkScanner::StopScan()
{
    wxCriticalSectionLocker lock(m_criticalSection);
    m_stopRequested = true;
}

wxThread::ExitCode NetworkScanner::Entry()
{
    LOG_INFO("=== Network Scanner Starting ===");
    LOG_INFO("Subnet: " + m_currentSubnet);
    
    // Get our own IP address first
    std::string localIP = GetLocalIP();
    LOG_INFO("Local IP: " + (localIP.empty() ? "(could not detect)" : localIP));
    
    // Step 1: Try ARP scan to find devices
    LOG_INFO("=== Step 1: ARP Scan ===");
    std::vector<NetworkDevice> devices = ArpScan(m_currentSubnet);
    LOG_INFO("ARP scan found " + std::to_string(devices.size()) + " devices");
    
    // Log all devices found (before filtering)
    LOG_INFO("=== All devices found by ARP scan ===");
    for (const auto& device : devices) {
        LOG_INFO("Found: " + device.ip + " (MAC: " + device.macAddress + ", Vendor: " + device.vendor + ")");
    }
    
    // Step 2: NO FILTERING - Show all devices found
    LOG_INFO("=== Step 2: NO FILTERING - Showing ALL devices found ===");
    LOG_INFO("Total devices to display: " + std::to_string(devices.size()));
    
    // Just log all devices that will be shown
    for (const auto& device : devices) {
        LOG_INFO("Showing device: " + device.ip + " (" + device.vendor + ")");
    }
    
    // Store results and notify completion
    {
        wxCriticalSectionLocker lock(m_criticalSection);
        m_devices = devices;
        m_isScanning = false;
    }
    
    if (m_completeCallback) {
        m_completeCallback(devices, true, "ARP scan completed - found " + std::to_string(devices.size()) + " devices");
    }
    
    return (wxThread::ExitCode)0;
}

std::vector<NetworkDevice> NetworkScanner::ScanWindows(const std::string& subnet)
{
    std::vector<NetworkDevice> devices;
    
    // First try ARP table scan (fast and finds devices that don't respond to ping)
    if (m_progressCallback) {
        m_progressCallback(10, 100, "", "Reading ARP table...");
    }
    auto arpDevices = ArpScan(subnet);
    devices.insert(devices.end(), arpDevices.begin(), arpDevices.end());
    
    if (m_stopRequested) return devices;
    
    // If we found devices in ARP, enhance them with additional info (but quickly)
    if (!devices.empty()) {
        if (m_progressCallback) {
            m_progressCallback(50, 100, "", "Enhancing device information...");
        }
        
        // Just classify device types based on vendor/MAC info - don't do network testing
        int deviceCount = devices.size();
        int currentDevice = 0;
        
        for (auto& device : devices) {
            if (m_stopRequested) break;
            
            currentDevice++;
            if (m_progressCallback) {
                int progress = 50 + (currentDevice * 40) / deviceCount; // 50% to 90%
                m_progressCallback(progress, 100, device.ip, "Classifying devices...");
            }
            
            // Use vendor/MAC information to enhance device classification
            if (!device.macAddress.empty() && device.vendor != "Unknown") {
                std::string enhancedType = MacVendorLookup::GetDeviceType(device.macAddress, device.vendor);
                if (enhancedType != "Unknown") {
                    device.deviceType = enhancedType;
                }
            }
            
            // Simple classification - don't exclude devices based on type
            if (device.deviceType == "Unknown") {
                device.deviceType = "Network Device";
            }
        }
        
        if (m_progressCallback) {
            m_progressCallback(100, 100, "", wxString::Format("Found %zu devices in ARP table", devices.size()).ToStdString());
        }
        return devices;
    }
    
    // Only do ping scan if ARP didn't find anything
    if (m_progressCallback) {
        m_progressCallback(30, 100, "", "ARP scan found no devices, starting ping scan...");
    }
    auto pingDevices = PingScan(subnet);
    
    // Merge results, avoiding duplicates
    for (const auto& pingDevice : pingDevices) {
        bool found = false;
        for (const auto& existingDevice : devices) {
            if (existingDevice.ip == pingDevice.ip) {
                found = true;
                break;
            }
        }
        if (!found) {
            devices.push_back(pingDevice);
        }
    }
    
    if (m_stopRequested) return devices;
    
    // Only do port scan if we still haven't found anything
    if (devices.empty()) {
        if (m_progressCallback) {
            m_progressCallback(60, 100, "", "No devices found, starting port scan...");
        }
        auto portScanDevices = PortScan(subnet);
        for (const auto& portDevice : portScanDevices) {
            bool found = false;
            for (const auto& existingDevice : devices) {
                if (existingDevice.ip == portDevice.ip) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                devices.push_back(portDevice);
            }
        }
    }
    
    return devices;
}

std::vector<NetworkDevice> NetworkScanner::ScanLinux(const std::string& subnet)
{
    // For Linux, use combination of ARP and ping
    std::vector<NetworkDevice> devices;
    
    auto arpDevices = ArpScan(subnet);
    devices.insert(devices.end(), arpDevices.begin(), arpDevices.end());
    
    auto pingDevices = PingScan(subnet);
    for (const auto& pingDevice : pingDevices) {
        bool found = false;
        for (const auto& existingDevice : devices) {
            if (existingDevice.ip == pingDevice.ip) {
                found = true;
                break;
            }
        }
        if (!found) {
            devices.push_back(pingDevice);
        }
    }
    
    return devices;
}

std::vector<NetworkDevice> NetworkScanner::ScanMacOS(const std::string& subnet)
{
    // Similar to Linux approach
    return ScanLinux(subnet);
}

std::vector<NetworkDevice> NetworkScanner::PingScan(const std::string& subnet)
{
    std::vector<NetworkDevice> devices;
    std::vector<std::string> ipRange = GenerateIPRange(subnet);
    
    int totalIPs = ipRange.size();
    int currentIP = 0;
    
    for (const std::string& ip : ipRange) {
        if (m_stopRequested) break;
        
        currentIP++;
        if (m_progressCallback) {
            m_progressCallback(currentIP, totalIPs, ip, "Pinging addresses...");
        }
        
        int responseTime;
        if (PingHost(ip, responseTime)) {
            NetworkDevice device;
            device.ip = ip;
            device.hostname = ResolveHostname(ip);
            device.macAddress = GetMacAddress(ip);
            device.vendor = GuessVendor(device.macAddress);
            device.isReachable = true;
            device.responseTime = responseTime;
            device.deviceType = GuessDeviceType(ip, device.hostname);
            
            LOG_DEBUG("Ping scan found device: " + ip + " (" + std::to_string(responseTime) + "ms) - " + device.deviceType);
            devices.push_back(device);
        }
        
        // Small delay to avoid overwhelming the network
        wxMilliSleep(10);
    }
    
    return devices;
}

std::vector<NetworkDevice> NetworkScanner::ArpScan(const std::string& subnet)
{
    std::vector<NetworkDevice> devices;
    
#ifdef _WIN32
    // Windows ARP table implementation
    PMIB_IPNETTABLE pIpNetTable = nullptr;
    DWORD dwSize = 0;
    
    // Get required buffer size
    GetIpNetTable(pIpNetTable, &dwSize, FALSE);
    pIpNetTable = (MIB_IPNETTABLE*)malloc(dwSize);
    
    if (pIpNetTable && GetIpNetTable(pIpNetTable, &dwSize, FALSE) == NO_ERROR) {
        DWORD totalEntries = pIpNetTable->dwNumEntries;
        DWORD processedEntries = 0;
        
        LOG_INFO("NetworkScanner: ARP table has " + std::to_string(totalEntries) + " total entries");
        
        for (DWORD i = 0; i < totalEntries && !m_stopRequested; i++) {
            MIB_IPNETROW& row = pIpNetTable->table[i];
            processedEntries++;
            
            // Update progress every 10 entries or at end
            if (processedEntries % 10 == 0 || processedEntries == totalEntries) {
                if (m_progressCallback) {
                    int progress = (processedEntries * 100) / totalEntries;
                    m_progressCallback(progress, 100, "", 
                        wxString::Format("Reading ARP table (%d/%d entries)", processedEntries, totalEntries).ToStdString());
                }
            }
            
            // Convert IP address
            struct in_addr addr;
            addr.S_un.S_addr = row.dwAddr;
            std::string ip = inet_ntoa(addr);
            
            // Skip if not in our subnet (extract network portion from subnet)
            size_t slashPos = subnet.find('/');
            std::string subnetBase = subnet;
            if (slashPos != std::string::npos) {
                subnetBase = subnet.substr(0, slashPos);
            }
            
            // Get network portion of both subnet and IP (assuming /24)
            size_t subnetLastDot = subnetBase.find_last_of('.');
            size_t ipLastDot = ip.find_last_of('.');
            
            if (subnetLastDot == std::string::npos || ipLastDot == std::string::npos) {
                continue; // Invalid format
            }
            
            std::string subnetNetwork = subnetBase.substr(0, subnetLastDot);
            std::string ipNetwork = ip.substr(0, ipLastDot);
            
            if (subnetNetwork != ipNetwork) {
                continue; // Not in same network
            }
            
            NetworkDevice device;
            device.ip = ip;
            device.hostname = ""; // Skip expensive hostname resolution during ARP scan
            device.isReachable = (row.dwType == MIB_IPNET_TYPE_DYNAMIC || row.dwType == MIB_IPNET_TYPE_STATIC);
            device.responseTime = -1; // Unknown from ARP table
            
            // Format MAC address
            if (row.dwPhysAddrLen == 6) {
                char macStr[18];
                snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                         row.bPhysAddr[0], row.bPhysAddr[1], row.bPhysAddr[2],
                         row.bPhysAddr[3], row.bPhysAddr[4], row.bPhysAddr[5]);
                device.macAddress = macStr;
            }
            
            device.vendor = GuessVendor(device.macAddress);
            device.deviceType = GuessDeviceType(ip, device.hostname, device.macAddress);
            
            // Special enhanced logic for devices with MAC vendor information
            if (!device.macAddress.empty() && device.vendor != "Unknown") {
                // Enhanced device type based on vendor and MAC
                std::string enhancedType = MacVendorLookup::GetDeviceType(device.macAddress, device.vendor);
                if (enhancedType != "Unknown") {
                    device.deviceType = enhancedType;
                }
            }
            
            devices.push_back(device);
            LOG_DEBUG("ARP scan found device: " + ip + " (" + device.macAddress + ") - " + device.vendor);
        }
    }
    
    if (pIpNetTable) {
        free(pIpNetTable);
    }
#else
    // Unix/Linux ARP implementation would go here
    // For now, return empty - will fall back to ping scan
#endif
    
    LOG_INFO("ARP scan complete, found " + std::to_string(devices.size()) + " devices");
    return devices;
}

std::vector<NetworkDevice> NetworkScanner::PortScan(const std::string& subnet)
{
    std::vector<NetworkDevice> devices;
    std::vector<std::string> ipRange = GenerateIPRange(subnet);
    
    // Common FluidNC/ESP32 ports to check
    std::vector<int> portsToCheck = {23, 80}; // Telnet and HTTP
    
    int totalTests = ipRange.size() * portsToCheck.size();
    int currentTest = 0;
    
    for (const std::string& ip : ipRange) {
        if (m_stopRequested) break;
        
        bool deviceFound = false;
        NetworkDevice device;
        device.ip = ip;
        device.hostname = ResolveHostname(ip);
        device.macAddress = "";
        device.vendor = "";
        device.isReachable = false;
        device.responseTime = -1;
        device.deviceType = "Unknown";
        
        for (int port : portsToCheck) {
            if (m_stopRequested) break;
            
            currentTest++;
            if (m_progressCallback) {
                m_progressCallback(currentTest, totalTests, ip, 
                    wxString::Format("Port scanning %s:%d...", ip, port).ToStdString());
            }
            
            if (TestTcpPort(ip, port)) {
                deviceFound = true;
                device.isReachable = true;
                
                // Special handling for specific ports
                if (port == 23) {
                    // Telnet port suggests FluidNC
                    device.deviceType = "FluidNC";
                } else if (port == 80) {
                    // HTTP port - could be various devices
                    if (device.deviceType == "Unknown") {
                        device.deviceType = "Web Device";
                    }
                }
            }
            
            // Small delay between port tests
            wxMilliSleep(5);
        }
        
        if (deviceFound) {
            // Final device type check using existing logic
            std::string finalType = GuessDeviceType(ip, device.hostname);
            if (finalType != "Unknown") {
                device.deviceType = finalType;
            }
            
            device.vendor = GuessVendor(device.macAddress);
            devices.push_back(device);
        }
        
        // Small delay between IP addresses
        wxMilliSleep(10);
    }
    
    return devices;
}

bool NetworkScanner::PingHost(const std::string& ip, int& responseTime)
{
#ifdef _WIN32
    // Windows ICMP ping implementation
    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    unsigned long ipaddr = inet_addr(ip.c_str());
    if (ipaddr == INADDR_NONE) {
        IcmpCloseHandle(hIcmpFile);
        return false;
    }
    
    char SendData[] = "FluidNC Scanner Ping";
    DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
    LPVOID ReplyBuffer = (VOID*)malloc(ReplySize);
    
    if (!ReplyBuffer) {
        IcmpCloseHandle(hIcmpFile);
        return false;
    }
    
    DWORD dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, SendData, sizeof(SendData),
                                  NULL, ReplyBuffer, ReplySize, 1000); // 1 second timeout
    
    bool success = false;
    if (dwRetVal != 0) {
        PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
        if (pEchoReply->Status == IP_SUCCESS) {
            responseTime = pEchoReply->RoundTripTime;
            success = true;
        }
    }
    
    free(ReplyBuffer);
    IcmpCloseHandle(hIcmpFile);
    return success;
#else
    // Unix ping implementation using socket would go here
    // For now, return false
    responseTime = -1;
    return false;
#endif
}

bool NetworkScanner::TestTcpPort(const std::string& ip, int port)
{
    // Test if a TCP port is open on the given IP address
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }
    
    // Set non-blocking mode for timeout control
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    // Try to connect
    int result = connect(sock, (sockaddr*)&addr, sizeof(addr));
    
    bool success = false;
    if (result == 0) {
        // Immediate connection
        success = true;
    } else if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            // Connection in progress, wait for completion
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(sock, &writeSet);
            
            struct timeval timeout;
            timeout.tv_sec = 0;  // 500ms timeout
            timeout.tv_usec = 500000;
            
            int selectResult = select(0, NULL, &writeSet, NULL, &timeout);
            if (selectResult > 0) {
                // Check if connection was successful
                int optval;
                int optlen = sizeof(optval);
                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&optval, &optlen) == 0) {
                    success = (optval == 0);
                }
            }
        }
    }
    
    closesocket(sock);
    WSACleanup();
    return success;
#else
    // Unix implementation using standard sockets
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    
    // Set non-blocking mode
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    // Try to connect
    int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    
    bool success = false;
    if (result == 0) {
        // Immediate connection
        success = true;
    } else if (errno == EINPROGRESS) {
        // Connection in progress, wait for completion
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);
        
        struct timeval timeout;
        timeout.tv_sec = 0;  // 500ms timeout
        timeout.tv_usec = 500000;
        
        int selectResult = select(sock + 1, NULL, &writeSet, NULL, &timeout);
        if (selectResult > 0) {
            // Check if connection was successful
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

std::string NetworkScanner::ResolveHostname(const std::string& ip)
{
    struct sockaddr_in sa;
    char host[NI_MAXHOST];
    
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);
    
    if (getnameinfo((struct sockaddr*)&sa, sizeof(sa), host, sizeof(host), NULL, 0, NI_NAMEREQD) == 0) {
        return std::string(host);
    }
    
    return "";
}

std::string NetworkScanner::GetMacAddress(const std::string& ip)
{
    // This would typically require ARP table lookup
    // For now, return empty string
    return "";
}

std::string NetworkScanner::GuessVendor(const std::string& macAddress)
{
    if (macAddress.empty()) {
        LOG_DEBUG("GuessVendor: Empty MAC address, returning Unknown");
        return "Unknown";
    }
    
    // Use the comprehensive MAC vendor lookup
    std::string vendor = MacVendorLookup::GetVendor(macAddress);
    LOG_DEBUG("GuessVendor: MAC=" + macAddress + " -> Vendor=" + vendor);
    return vendor;
}

std::string NetworkScanner::GuessDeviceType(const std::string& ip, const std::string& hostname)
{
    // Enhanced device type detection that incorporates vendor information
    return GuessDeviceType(ip, hostname, "");
}

std::string NetworkScanner::GuessDeviceType(const std::string& ip, const std::string& hostname, const std::string& macAddress)
{
    // Get vendor information if MAC address is available
    std::string vendor = "Unknown";
    if (!macAddress.empty()) {
        vendor = MacVendorLookup::GetVendor(macAddress);
        
        // Use MacVendorLookup's enhanced device type detection
        std::string vendorBasedType = MacVendorLookup::GetDeviceType(macAddress, vendor);
        if (vendorBasedType != "Unknown") {
            // For ESP32/ESP8266 devices, check for FluidNC specifically
            if (vendorBasedType == "ESP32/ESP8266" && IsFluidNCDevice(ip)) {
                return "FluidNC";
            }
            return vendorBasedType;
        }
    }
    
    // Check if it might be FluidNC by attempting telnet connection
    if (IsFluidNCDevice(ip)) {
        return "FluidNC";
    }
    
    // Check hostname patterns
    std::string lowerHostname = hostname;
    std::transform(lowerHostname.begin(), lowerHostname.end(), lowerHostname.begin(), ::tolower);
    
    if (lowerHostname.find("router") != std::string::npos || 
        lowerHostname.find("gateway") != std::string::npos ||
        ip.find(".1") == ip.length() - 2) { // Common gateway IP pattern
        return "Router";
    }
    
    if (lowerHostname.find("esp") != std::string::npos ||
        lowerHostname.find("arduino") != std::string::npos) {
        return "ESP32/ESP8266";
    }
    
    // Fall back to vendor-based guess if we have vendor info
    if (vendor != "Unknown") {
        std::string lowerVendor = vendor;
        std::transform(lowerVendor.begin(), lowerVendor.end(), lowerVendor.begin(), ::tolower);
        
        if (lowerVendor.find("espressif") != std::string::npos) {
            return "ESP32/ESP8266";
        }
    }
    
    return "Unknown";
}

std::vector<std::string> NetworkScanner::GenerateIPRange(const std::string& subnet)
{
    std::vector<std::string> ipRange;
    
    if (subnet.empty()) return ipRange;
    
    // Simple subnet parsing for /24 networks (e.g., "192.168.1.0/24")
    size_t slashPos = subnet.find('/');
    std::string baseIP = subnet;
    
    if (slashPos != std::string::npos) {
        baseIP = subnet.substr(0, slashPos);
    }
    
    // Extract network portion (assuming /24)
    size_t lastDotPos = baseIP.find_last_of('.');
    if (lastDotPos != std::string::npos) {
        std::string networkPortion = baseIP.substr(0, lastDotPos);
        
        // Generate all IPs in the range
        for (int i = 1; i <= 254; i++) {
            ipRange.push_back(networkPortion + "." + std::to_string(i));
        }
    }
    
    return ipRange;
}

bool NetworkScanner::IsFluidNCDevice(const std::string& ip)
{
    // Try to connect to telnet port (23) to see if it responds like FluidNC
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }
    
    // Set non-blocking mode for timeout
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(23); // Telnet port
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    // Try to connect
    int result = connect(sock, (sockaddr*)&addr, sizeof(addr));
    
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            // Wait for connection with short timeout
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(sock, &writeSet);
            
            struct timeval timeout;
            timeout.tv_sec = 1;  // 1 second timeout
            timeout.tv_usec = 0;
            
            int selectResult = select(0, NULL, &writeSet, NULL, &timeout);
            if (selectResult > 0) {
                // Check if connection was successful
                int optval;
                int optlen = sizeof(optval);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&optval, &optlen);
                
                closesocket(sock);
                WSACleanup();
                return (optval == 0);
            }
        }
    }
    
    closesocket(sock);
    WSACleanup();
    return false;
#else
    return false; // Unix implementation would go here
#endif
}

// WinsockHelper implementation
#ifdef _WIN32
NetworkScanner::WinsockHelper::WinsockHelper() : m_initialized(false)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
        m_initialized = true;
    }
}

NetworkScanner::WinsockHelper::~WinsockHelper()
{
    if (m_initialized) {
        WSACleanup();
        m_initialized = false;
    }
}
#else
NetworkScanner::WinsockHelper::WinsockHelper() : m_initialized(true) {}
NetworkScanner::WinsockHelper::~WinsockHelper() {}
#endif

bool NetworkScanner::TestTcpPortWithHelper(const std::string& ip, int port, WinsockHelper& winsock)
{
    if (!winsock.IsInitialized()) {
        return false;
    }
    
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return false;
    }
    
    // Set non-blocking mode for timeout control
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    // Try to connect
    int result = connect(sock, (sockaddr*)&addr, sizeof(addr));
    
    bool success = false;
    if (result == 0) {
        // Immediate connection
        success = true;
    } else if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            // Connection in progress, wait for completion
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(sock, &writeSet);
            
            struct timeval timeout;
            timeout.tv_sec = 0;  // 300ms timeout (faster than original)
            timeout.tv_usec = 300000;
            
            int selectResult = select(0, NULL, &writeSet, NULL, &timeout);
            if (selectResult > 0) {
                // Check if connection was successful
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
    // Unix implementation using standard sockets
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    
    // Set non-blocking mode
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    // Try to connect
    int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    
    bool success = false;
    if (result == 0) {
        // Immediate connection
        success = true;
    } else if (errno == EINPROGRESS) {
        // Connection in progress, wait for completion
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);
        
        struct timeval timeout;
        timeout.tv_sec = 0;  // 300ms timeout
        timeout.tv_usec = 300000;
        
        int selectResult = select(sock + 1, NULL, &writeSet, NULL, &timeout);
        if (selectResult > 0) {
            // Check if connection was successful
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

std::string NetworkScanner::GetPlatform()
{
#ifdef _WIN32
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#else
    return "Unknown";
#endif
}

std::vector<std::pair<std::string, std::string>> NetworkScanner::GetPhysicalNetworkAdapters()
{
    std::vector<std::pair<std::string, std::string>> adapters; // IP, Subnet pairs
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return adapters;
    }
    
    // Get adapter information using GetAdaptersInfo
    ULONG bufferLength = 0;
    GetAdaptersInfo(nullptr, &bufferLength);
    
    if (bufferLength > 0) {
        PIP_ADAPTER_INFO adapterInfo = (PIP_ADAPTER_INFO)malloc(bufferLength);
        if (adapterInfo && GetAdaptersInfo(adapterInfo, &bufferLength) == ERROR_SUCCESS) {
            PIP_ADAPTER_INFO adapter = adapterInfo;
            
            while (adapter) {
                // Only include Ethernet and WiFi adapters
                if (adapter->Type == MIB_IF_TYPE_ETHERNET || adapter->Type == IF_TYPE_IEEE80211) { // Ethernet or WiFi
                    
                    std::string ipStr = adapter->IpAddressList.IpAddress.String;
                    std::string maskStr = adapter->IpAddressList.IpMask.String;
                    
                    // Skip invalid, loopback, or APIPA addresses
                    if (ipStr != "0.0.0.0" && ipStr != "127.0.0.1" && !ipStr.empty() && 
                        ipStr.find("169.254.") != 0) { // Skip APIPA addresses
                        
                        // Calculate network address
                        struct in_addr ip, mask, network;
                        inet_pton(AF_INET, ipStr.c_str(), &ip);
                        inet_pton(AF_INET, maskStr.c_str(), &mask);
                        
                        network.S_un.S_addr = ip.S_un.S_addr & mask.S_un.S_addr;
                        
                        // Convert network mask to CIDR notation
                        int cidr = 0;
                        uint32_t maskVal = ntohl(mask.S_un.S_addr);
                        while (maskVal & 0x80000000) {
                            cidr++;
                            maskVal <<= 1;
                        }
                        
                        std::string networkStr = inet_ntoa(network);
                        std::string subnet = networkStr + "/" + std::to_string(cidr);
                        
                        adapters.push_back(std::make_pair(ipStr, subnet));
                        
                        std::string adapterTypeStr = (adapter->Type == MIB_IF_TYPE_ETHERNET) ? "Ethernet" : "WiFi";
                        LOG_INFO("Found physical adapter: " + adapterTypeStr + " - IP: " + ipStr + ", Subnet: " + subnet + " (" + std::string(adapter->AdapterName) + ")");
                    }
                }
                adapter = adapter->Next;
            }
        }
        
        if (adapterInfo) {
            free(adapterInfo);
        }
    }
    
    WSACleanup();
#endif
    
    return adapters;
}

std::string NetworkScanner::GetLocalIP()
{
    // Get the local IP from physical adapters (not VPN)
    auto adapters = GetPhysicalNetworkAdapters();
    if (!adapters.empty()) {
        // Return the first physical adapter IP
        LOG_INFO("Using physical adapter IP: " + adapters[0].first);
        return adapters[0].first;
    }
    return ""; // Can't determine
}

std::string NetworkScanner::GetLocalSubnet()
{
    // Use physical adapters to get the correct subnet
    auto adapters = GetPhysicalNetworkAdapters();
    if (!adapters.empty()) {
        // Return the first physical adapter subnet
        LOG_INFO("Using physical adapter subnet: " + adapters[0].second);
        return adapters[0].second;
    }
    
    // Ultimate fallback - try common private network ranges
    LOG_ERROR("NetworkScanner: Could not detect local subnet, using default fallback");
    return "192.168.1.0/24";
}
