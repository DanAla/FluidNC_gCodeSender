/**
 * core/NetworkScanner.h
 * Cross-platform network scanner for device discovery
 * Supports Windows, Linux, and macOS with appropriate fallback methods
 */

#pragma once

#include <wx/wx.h>
#include <wx/thread.h>
#include <vector>
#include <string>
#include <functional>
#include <memory>

/**
 * Discovered network device information
 */
struct NetworkDevice {
    std::string ip;
    std::string hostname;
    std::string macAddress;
    std::string vendor;
    bool isReachable;
    int responseTime; // in milliseconds
    std::string deviceType; // "Unknown", "Router", "FluidNC", etc.
};

/**
 * Network scanning progress callback
 * Parameters: currentStep, totalSteps, currentIP, message
 */
using ScanProgressCallback = std::function<void(int, int, const std::string&, const std::string&)>;

/**
 * Network scanning completion callback
 * Parameters: devices, success, errorMessage
 */
using ScanCompleteCallback = std::function<void(const std::vector<NetworkDevice>&, bool, const std::string&)>;

/**
 * Cross-platform network scanner
 * Automatically detects platform and uses appropriate scanning methods
 */
class NetworkScanner : public wxThread {
public:
    NetworkScanner();
    virtual ~NetworkScanner();

    // Scanning control
    void StartScan(const std::string& subnet = "");
    void StopScan();
    bool IsScanning() const { return m_isScanning; }

    // Callbacks
    void SetProgressCallback(ScanProgressCallback callback) { m_progressCallback = callback; }
    void SetCompleteCallback(ScanCompleteCallback callback) { m_completeCallback = callback; }

protected:
    // Thread entry point
    virtual ExitCode Entry() override;

private:
    // Device scanning and classification
    std::vector<NetworkDevice> ScanDevices(const std::string& subnet);
    std::string GuessVendor(const std::string& macAddress);
    std::string GuessDeviceType(const std::string& ip, const std::string& hostname);
    std::string GuessDeviceType(const std::string& ip, const std::string& hostname, const std::string& macAddress);
    
    // Subnet utilities
    std::vector<std::string> GenerateIPRange(const std::string& subnet);
    
    // Threading and state
    volatile bool m_isScanning;
    volatile bool m_stopRequested;
    wxCriticalSection m_criticalSection;
    
    // Callbacks
    ScanProgressCallback m_progressCallback;
    ScanCompleteCallback m_completeCallback;
    
    // Current scan parameters
    std::string m_currentSubnet;
    
    // Results
    std::vector<NetworkDevice> m_devices;
};
