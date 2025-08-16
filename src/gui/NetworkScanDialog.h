/**
 * gui/NetworkScanDialog.h
 * Network scanning dialog for discovering FluidNC and other devices
 * Shows real-time progress and results in a user-friendly interface
 */

#pragma once

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/gauge.h>
#include <wx/timer.h>
#include <memory>
#include "../core/NetworkScanner.h"

/**
 * Network Scan Dialog
 * Displays network scanning progress and results
 * Allows user to select discovered devices for machine configuration
 */
class NetworkScanDialog : public wxDialog {
public:
    NetworkScanDialog(wxWindow* parent);
    virtual ~NetworkScanDialog();

    // Get selected device (if any) when dialog is closed with OK
    NetworkDevice GetSelectedDevice() const;
    bool HasSelectedDevice() const { return m_hasSelectedDevice; }

private:
    // Scanner cleanup
    void CleanupScanner();
    
    // Event handlers
    void OnStartScan(wxCommandEvent& event);
    void OnStopScan(wxCommandEvent& event);
    void OnRescan(wxCommandEvent& event);
    void OnDeviceSelected(wxListEvent& event);
    void OnDeviceActivated(wxListEvent& event);
    void OnUseSelected(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnTimer(wxTimerEvent& event);

    // UI Creation
    void CreateControls();
    void CreateDeviceList();
    wxSizer* CreateProgressControls();
    wxSizer* CreateButtonPanel();

    // Scanning callbacks (thread-safe)
    void OnScanProgress(int current, int total, const std::string& currentIP, const std::string& message);
    void OnScanComplete(const std::vector<NetworkDevice>& devices, bool success, const std::string& error);

    // UI Updates (must be called from main thread)
    void UpdateProgress(int current, int total, const wxString& currentIP, const wxString& message);
    void UpdateDeviceList(const std::vector<NetworkDevice>& devices);
    void SetScanningState(bool scanning);

    // Device list management
    void PopulateDeviceList();
    void AddDeviceToList(const NetworkDevice& device);
    void ClearDeviceList();
    
    // Utility methods
    wxString FormatDeviceType(const std::string& deviceType);
    wxString FormatResponseTime(int responseTime);
    wxColour GetStatusColor(bool isReachable);
    wxString GetSubnetInfo();

private:
    // UI Components
    wxStaticText* m_titleLabel;
    wxStaticText* m_subnetLabel;
    wxStaticText* m_statusLabel;
    
    wxGauge* m_progressBar;
    wxStaticText* m_progressLabel;
    wxStaticText* m_currentIPLabel;
    
    wxListCtrl* m_deviceList;
    
    wxButton* m_startBtn;
    wxButton* m_stopBtn;
    wxButton* m_rescanBtn;
    wxButton* m_useSelectedBtn;
    wxButton* m_cancelBtn;

    // Scanner
    std::unique_ptr<NetworkScanner> m_scanner;
    
    // State
    bool m_isScanning;
    bool m_hasSelectedDevice;
    NetworkDevice m_selectedDevice;
    std::vector<NetworkDevice> m_discoveredDevices;
    
    // Timer for UI updates
    wxTimer m_updateTimer;
    
    // Thread-safe communication
    struct ProgressUpdate {
        int current;
        int total;
        wxString currentIP;
        wxString message;
    };
    
    struct DeviceUpdate {
        std::vector<NetworkDevice> devices;
        bool success;
        wxString error;
    };
    
    wxCriticalSection m_updateSection;
    std::vector<ProgressUpdate> m_progressUpdates;
    std::vector<DeviceUpdate> m_deviceUpdates;

    wxDECLARE_EVENT_TABLE();
};
