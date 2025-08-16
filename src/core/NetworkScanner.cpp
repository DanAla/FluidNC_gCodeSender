/**
 * core/NetworkScanner.cpp
 * Cross-platform network scanner implementation
 */

#include "NetworkScanner.h"
#include "NetworkManager.h"
#include "SimpleLogger.h"
#include "MacVendorLookup.h"
#include <wx/log.h>
#include <wx/utils.h>
#include <algorithm>
#include <sstream>
#include <regex>

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
    if (m_isScanning) {
        LOG_INFO("Network scan already in progress, ignoring new request");
        return;
    }
    
{
        wxCriticalSectionLocker lock(m_criticalSection);
        m_isScanning = true;
        m_stopRequested = false;
        // If no subnet specified, use first adapter's subnet
        if (subnet.empty()) {
            auto adapters = NetworkManager::getInstance().getNetworkAdapters();
            if (!adapters.empty()) {
                m_currentSubnet = adapters[0].second; // second is the subnet
            } else {
                m_currentSubnet = "192.168.1.0/24"; // fallback
            }
        } else {
            m_currentSubnet = subnet;
        }
        m_devices.clear();
    }
    
    // Log scan start info
    LOG_INFO("=== Network Scan Started ===");
    LOG_INFO("Scanning subnet: " + m_currentSubnet);
    
    // Start the scanning thread
    if (Create() == wxTHREAD_NO_ERROR) {
        Run();
    } else {
        LOG_ERROR("Failed to create network scanning thread");
        m_isScanning = false;
    }
}

void NetworkScanner::StopScan()
{
    wxCriticalSectionLocker lock(m_criticalSection);
    m_stopRequested = true;
    
    // Stop any ongoing network operations
    NetworkManager::getInstance().cleanup();
}

wxThread::ExitCode NetworkScanner::Entry()
{
    LOG_INFO("=== Network Scanner Starting ===");
    LOG_INFO("Subnet: " + m_currentSubnet);
    
    // Use the unified scan method
    LOG_INFO("=== Starting Network Scan ===");
    auto devices = ScanDevices(m_currentSubnet);

    LOG_INFO("=== All devices found ===");
    for (const auto& device : devices) {
        LOG_INFO("Found: " + device.ip + 
                 " (MAC: " + (device.macAddress.empty() ? "unknown" : device.macAddress) + 
                 ", Type: " + device.deviceType + 
                 ", Vendor: " + device.vendor + ")");
    }
    
    LOG_INFO("Total devices found: " + std::to_string(devices.size()));
    
    // Store results and notify completion
    {
        wxCriticalSectionLocker lock(m_criticalSection);
        m_devices = devices;
        m_isScanning = false;
    }
    
    if (m_completeCallback) {
        m_completeCallback(devices, true, "Network scan completed - found " + std::to_string(devices.size()) + " devices");
    }
    
    return (wxThread::ExitCode)0;
}

std::vector<NetworkDevice> NetworkScanner::ScanDevices(const std::string& subnet)
{
    auto& netman = NetworkManager::getInstance();
    if (!netman.initialize()) {
        LOG_ERROR("Failed to initialize network manager");
        return {};
    }
    
    if (m_progressCallback) {
        m_progressCallback(0, 100, "", "Reading ARP table...");
    }

    // Get adapters to find devices
    auto adapters = netman.getNetworkAdapters();
    if (adapters.empty()) {
        LOG_ERROR("No network adapters found");
        if (m_progressCallback) {
            m_progressCallback(100, 100, "", "No network adapters found");
        }
        return {};
    }
    
    // Process each device in the subnet
    std::vector<NetworkDevice> devices;
    std::vector<std::string> ipRange = GenerateIPRange(subnet);
    
    int totalIPs = ipRange.size();
    int currentIP = 0;
    
    for (const std::string& ip : ipRange) {
        if (m_stopRequested) break;
        
        currentIP++;
        if (m_progressCallback) {
            m_progressCallback((currentIP * 100) / totalIPs, 100, ip, "Checking address...");
        }
        
        // Try telnet port first (for FluidNC)
        if (netman.testTcpPort(ip, 23)) {
            NetworkDevice device;
            device.ip = ip;
            device.hostname = netman.resolveHostname(ip);
            device.isReachable = true;
            device.responseTime = -1;
            device.deviceType = "FluidNC";
            devices.push_back(device);
            continue;
        }
        
        // Try HTTP port next (common for network devices)
        if (netman.testTcpPort(ip, 80)) {
            NetworkDevice device;
            device.ip = ip;
            device.hostname = netman.resolveHostname(ip);
            device.isReachable = true;
            device.responseTime = -1;
            device.deviceType = "Web Device";
            devices.push_back(device);
            continue;
        }
        
        // Finally try a ping
        int responseTime;
        if (netman.sendPing(ip, responseTime)) {
            NetworkDevice device;
            device.ip = ip;
            device.hostname = netman.resolveHostname(ip);
            device.isReachable = true;
            device.responseTime = responseTime;
            device.deviceType = GuessDeviceType(ip, device.hostname);
            devices.push_back(device);
        }
        
        // Small delay between devices
        wxMilliSleep(5);
    }
    
    LOG_INFO("Found " + std::to_string(devices.size()) + " devices");
    return devices;
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
            if (vendorBasedType == "ESP32/ESP8266" && NetworkManager::getInstance().testTcpPort(ip, 23)) {
                return "FluidNC";
            }
            return vendorBasedType;
        }
    }
    
    // Check if it might be FluidNC by attempting telnet connection
    if (NetworkManager::getInstance().testTcpPort(ip, 23)) {
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
