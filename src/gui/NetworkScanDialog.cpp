/**
 * gui/NetworkScanDialog.cpp
 * Network scanning dialog implementation
 */

#include "NetworkScanDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include "../core/SimpleLogger.h"
#include "NotificationSystem.h"

// Control IDs for NetworkScanDialog
enum {
    ID_START_SCAN = wxID_HIGHEST + 3000,
    ID_STOP_SCAN,
    ID_RESCAN,
    ID_DEVICE_LIST,
    ID_USE_SELECTED,
    ID_TIMER_UPDATE
};

wxBEGIN_EVENT_TABLE(NetworkScanDialog, wxDialog)
    EVT_BUTTON(ID_START_SCAN, NetworkScanDialog::OnStartScan)
    EVT_BUTTON(ID_STOP_SCAN, NetworkScanDialog::OnStopScan)
    EVT_BUTTON(ID_RESCAN, NetworkScanDialog::OnRescan)
    EVT_LIST_ITEM_SELECTED(ID_DEVICE_LIST, NetworkScanDialog::OnDeviceSelected)
    EVT_LIST_ITEM_ACTIVATED(ID_DEVICE_LIST, NetworkScanDialog::OnDeviceActivated)
    EVT_BUTTON(ID_USE_SELECTED, NetworkScanDialog::OnUseSelected)
    EVT_BUTTON(wxID_CANCEL, NetworkScanDialog::OnCancel)
    EVT_CLOSE(NetworkScanDialog::OnClose)
    EVT_TIMER(ID_TIMER_UPDATE, NetworkScanDialog::OnTimer)
wxEND_EVENT_TABLE()

NetworkScanDialog::NetworkScanDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Network Scanner", wxDefaultPosition, wxSize(800, 600),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_scanner(nullptr),
      m_isScanning(false),
      m_hasSelectedDevice(false),
      m_updateTimer(this, ID_TIMER_UPDATE)
{
    CreateControls();
    CenterOnParent();
    
    // Start scanning automatically
    wxCommandEvent evt;
    OnStartScan(evt);
}

NetworkScanDialog::~NetworkScanDialog()
{
    if (m_scanner && m_isScanning) {
        m_scanner->StopScan();
        m_scanner->Wait();
    }
}

NetworkDevice NetworkScanDialog::GetSelectedDevice() const
{
    return m_selectedDevice;
}

void NetworkScanDialog::CreateControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title and subnet info
    m_titleLabel = new wxStaticText(this, wxID_ANY, "Network Device Scanner");
    m_titleLabel->SetFont(m_titleLabel->GetFont().Scale(1.3).Bold());
    mainSizer->Add(m_titleLabel, 0, wxALL | wxCENTER, 10);
    
    m_subnetLabel = new wxStaticText(this, wxID_ANY, GetSubnetInfo());
    mainSizer->Add(m_subnetLabel, 0, wxALL | wxCENTER, 5);
    
    wxSizer* progressSizer = CreateProgressControls();
    mainSizer->Add(progressSizer, 0, wxALL | wxEXPAND, 10);
    
    CreateDeviceList();
    mainSizer->Add(m_deviceList, 1, wxALL | wxEXPAND, 10);
    
    wxSizer* buttonSizer = CreateButtonPanel();
    mainSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 10);
    
    SetSizer(mainSizer);
}

wxSizer* NetworkScanDialog::CreateProgressControls()
{
    wxStaticBoxSizer* progressBox = new wxStaticBoxSizer(wxVERTICAL, this, "Scan Progress");
    
    m_statusLabel = new wxStaticText(this, wxID_ANY, "Ready to scan");
    progressBox->Add(m_statusLabel, 0, wxALL | wxEXPAND, 5);
    
    m_progressBar = new wxGauge(this, wxID_ANY, 100);
    progressBox->Add(m_progressBar, 0, wxALL | wxEXPAND, 5);
    
    m_progressLabel = new wxStaticText(this, wxID_ANY, "0 / 0 addresses scanned");
    progressBox->Add(m_progressLabel, 0, wxALL | wxEXPAND, 5);
    
    m_currentIPLabel = new wxStaticText(this, wxID_ANY, "");
    progressBox->Add(m_currentIPLabel, 0, wxALL | wxEXPAND, 5);
    
    return progressBox;
}

void NetworkScanDialog::CreateDeviceList()
{
    m_deviceList = new wxListCtrl(this, ID_DEVICE_LIST, wxDefaultPosition, wxDefaultSize,
                                  wxLC_REPORT | wxLC_SINGLE_SEL);
    
    m_deviceList->AppendColumn("IP Address", wxLIST_FORMAT_LEFT, 120);
    m_deviceList->AppendColumn("Hostname", wxLIST_FORMAT_LEFT, 150);
    m_deviceList->AppendColumn("Device Type", wxLIST_FORMAT_LEFT, 100);
    m_deviceList->AppendColumn("Response Time", wxLIST_FORMAT_RIGHT, 100);
    m_deviceList->AppendColumn("MAC Address", wxLIST_FORMAT_LEFT, 130);
    m_deviceList->AppendColumn("Vendor", wxLIST_FORMAT_LEFT, 120);
}

wxSizer* NetworkScanDialog::CreateButtonPanel()
{
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_startBtn = new wxButton(this, ID_START_SCAN, "Start Scan");
    m_stopBtn = new wxButton(this, ID_STOP_SCAN, "Stop Scan");
    m_rescanBtn = new wxButton(this, ID_RESCAN, "Rescan");
    
    buttonSizer->Add(m_startBtn, 0, wxRIGHT, 5);
    buttonSizer->Add(m_stopBtn, 0, wxRIGHT, 5);
    buttonSizer->Add(m_rescanBtn, 0, wxRIGHT, 10);
    
    buttonSizer->AddStretchSpacer();
    
    m_useSelectedBtn = new wxButton(this, ID_USE_SELECTED, "Use Selected Device");
    m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    
    buttonSizer->Add(m_useSelectedBtn, 0, wxRIGHT, 5);
    buttonSizer->Add(m_cancelBtn, 0);
    
    // Initial button states
    m_stopBtn->Enable(false);
    m_useSelectedBtn->Enable(false);
    
    return buttonSizer;
}

void NetworkScanDialog::OnStartScan(wxCommandEvent& WXUNUSED(event))
{
    if (m_isScanning) return;
    
    SetScanningState(true);
    ClearDeviceList();
    
    // Create and start scanner
    m_scanner = std::make_unique<NetworkScanner>();
    
    // Set up callbacks
    m_scanner->SetProgressCallback([this](int current, int total, const std::string& currentIP, const std::string& message) {
        // Thread-safe progress update
        wxCriticalSectionLocker lock(m_updateSection);
        ProgressUpdate update;
        update.current = current;
        update.total = total;
        update.currentIP = wxString(currentIP);
        update.message = wxString(message);
        m_progressUpdates.push_back(update);
    });
    
    m_scanner->SetCompleteCallback([this](const std::vector<NetworkDevice>& devices, bool success, const std::string& error) {
        // Thread-safe completion update
        wxCriticalSectionLocker lock(m_updateSection);
        DeviceUpdate update;
        update.devices = devices;
        update.success = success;
        update.error = wxString(error);
        m_deviceUpdates.push_back(update);
    });
    
    m_scanner->StartScan();
    
    // Start UI update timer
    m_updateTimer.Start(100); // Update every 100ms
}

void NetworkScanDialog::OnStopScan(wxCommandEvent& WXUNUSED(event))
{
    if (!m_isScanning) return;
    
    if (m_scanner) {
        m_scanner->StopScan();
    }
    
    SetScanningState(false);
}

void NetworkScanDialog::OnRescan(wxCommandEvent& WXUNUSED(event))
{
    if (m_isScanning) {
        if (m_scanner) {
            m_scanner->StopScan();
        }
        SetScanningState(false);
    }
    
    wxMilliSleep(500); // Brief pause
    
    // Restart scan
    if (!m_isScanning) {
        SetScanningState(true);
        ClearDeviceList();
        
        m_scanner = std::make_unique<NetworkScanner>();
        
        // Set up callbacks
        m_scanner->SetProgressCallback([this](int current, int total, const std::string& currentIP, const std::string& message) {
            wxCriticalSectionLocker lock(m_updateSection);
            ProgressUpdate update;
            update.current = current;
            update.total = total;
            update.currentIP = wxString(currentIP);
            update.message = wxString(message);
            m_progressUpdates.push_back(update);
        });
        
        m_scanner->SetCompleteCallback([this](const std::vector<NetworkDevice>& devices, bool success, const std::string& error) {
            wxCriticalSectionLocker lock(m_updateSection);
            DeviceUpdate update;
            update.devices = devices;
            update.success = success;
            update.error = wxString(error);
            m_deviceUpdates.push_back(update);
        });
        
        m_scanner->StartScan();
        m_updateTimer.Start(100);
    }
}

void NetworkScanDialog::OnDeviceSelected(wxListEvent& event)
{
    long selectedIndex = event.GetIndex();
    if (selectedIndex >= 0 && selectedIndex < (long)m_discoveredDevices.size()) {
        m_selectedDevice = m_discoveredDevices[selectedIndex];
        m_hasSelectedDevice = true;
        m_useSelectedBtn->Enable(true);
    }
}

void NetworkScanDialog::OnDeviceActivated(wxListEvent& WXUNUSED(event))
{
    if (m_hasSelectedDevice) {
        EndModal(wxID_OK);
    }
}

void NetworkScanDialog::OnUseSelected(wxCommandEvent& WXUNUSED(event))
{
    if (m_hasSelectedDevice) {
        EndModal(wxID_OK);
    }
}

void NetworkScanDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    if (m_isScanning && m_scanner) {
        m_scanner->StopScan();
        SetScanningState(false);
    }
    EndModal(wxID_CANCEL);
}

void NetworkScanDialog::OnClose(wxCloseEvent& WXUNUSED(event))
{
    if (m_isScanning && m_scanner) {
        m_scanner->StopScan();
        SetScanningState(false);
    }
    EndModal(wxID_CANCEL);
}

void NetworkScanDialog::OnTimer(wxTimerEvent& WXUNUSED(event))
{
    // Process pending updates
    wxCriticalSectionLocker lock(m_updateSection);
    
    // Process progress updates
    for (const auto& update : m_progressUpdates) {
        UpdateProgress(update.current, update.total, update.currentIP, update.message);
    }
    m_progressUpdates.clear();
    
    // Process device updates
    if (!m_deviceUpdates.empty()) {
        LOG_DEBUG("OnTimer: Processing " + std::to_string(m_deviceUpdates.size()) + " device updates");
    }
    for (const auto& update : m_deviceUpdates) {
        LOG_DEBUG("OnTimer: Device update - success=" + std::string(update.success ? "true" : "false") + 
                 ", devices=" + std::to_string(update.devices.size()) + 
                 ", error=" + update.error.ToStdString());
        UpdateDeviceList(update.devices);
        if (!update.success && !update.error.IsEmpty()) {
            NotificationSystem::Instance().ShowError("Scan Error", update.error);
        }
        SetScanningState(false);
    }
    m_deviceUpdates.clear();
}

void NetworkScanDialog::UpdateProgress(int current, int total, const wxString& currentIP, const wxString& message)
{
    if (total > 0) {
        int percent = (current * 100) / total;
        m_progressBar->SetValue(percent);
        m_progressLabel->SetLabel(wxString::Format("%d / %d addresses scanned", current, total));
    }
    
    m_statusLabel->SetLabel(message);
    m_currentIPLabel->SetLabel(currentIP.IsEmpty() ? "" : "Scanning: " + currentIP);
}

void NetworkScanDialog::UpdateDeviceList(const std::vector<NetworkDevice>& devices)
{
    // Debug: Log what devices are being received using same logger as NetworkScanner
    LOG_DEBUG("NetworkScanDialog: UpdateDeviceList called with " + std::to_string(devices.size()) + " devices");
    for (size_t i = 0; i < devices.size(); ++i) {
        std::string deviceInfo = "UI Device " + std::to_string(i) + ": IP=" + devices[i].ip + 
                                ", Type=" + devices[i].deviceType + 
                                ", Hostname=" + (devices[i].hostname.empty() ? "(none)" : devices[i].hostname);
        LOG_DEBUG(deviceInfo);
    }
    
    m_discoveredDevices = devices;
    PopulateDeviceList();
}

void NetworkScanDialog::SetScanningState(bool scanning)
{
    m_isScanning = scanning;
    
    m_startBtn->Enable(!scanning);
    m_stopBtn->Enable(scanning);
    m_rescanBtn->Enable(!scanning);
    
    if (!scanning) {
        m_updateTimer.Stop();
        m_statusLabel->SetLabel("Scan completed");
        m_currentIPLabel->SetLabel("");
        m_progressBar->SetValue(100);
    }
}

void NetworkScanDialog::PopulateDeviceList()
{
    LOG_DEBUG("NetworkScanDialog: PopulateDeviceList called, clearing list and adding " + std::to_string(m_discoveredDevices.size()) + " devices");
    
    // Clear only the UI list control, NOT the m_discoveredDevices vector
    m_deviceList->DeleteAllItems();
    m_hasSelectedDevice = false;
    m_useSelectedBtn->Enable(false);
    
    for (size_t i = 0; i < m_discoveredDevices.size(); ++i) {
        LOG_DEBUG("Adding device " + std::to_string(i) + " to UI list: " + m_discoveredDevices[i].ip);
        AddDeviceToList(m_discoveredDevices[i]);
    }
    
    // Update status
    wxString statusMsg = wxString::Format("Found %zu devices on the network", m_discoveredDevices.size());
    m_statusLabel->SetLabel(statusMsg);
    LOG_DEBUG("NetworkScanDialog: Device list populated with " + std::to_string(m_deviceList->GetItemCount()) + " items in UI");
}

void NetworkScanDialog::AddDeviceToList(const NetworkDevice& device)
{
    long index = m_deviceList->InsertItem(m_deviceList->GetItemCount(), device.ip);
    m_deviceList->SetItem(index, 1, device.hostname.empty() ? "-" : device.hostname);
    m_deviceList->SetItem(index, 2, FormatDeviceType(device.deviceType));
    m_deviceList->SetItem(index, 3, FormatResponseTime(device.responseTime));
    m_deviceList->SetItem(index, 4, device.macAddress.empty() ? "-" : device.macAddress);
    m_deviceList->SetItem(index, 5, device.vendor.empty() ? "-" : device.vendor);
    
    // Color coding based on device type
    if (device.deviceType == "FluidNC") {
        m_deviceList->SetItemTextColour(index, wxColour(0, 128, 0)); // Dark green for FluidNC
    } else if (device.deviceType == "Router" || device.deviceType == "Gateway") {
        m_deviceList->SetItemTextColour(index, wxColour(0, 0, 128)); // Dark blue for network equipment
    }
}

void NetworkScanDialog::ClearDeviceList()
{
    m_deviceList->DeleteAllItems();
    m_discoveredDevices.clear();
    m_hasSelectedDevice = false;
    m_useSelectedBtn->Enable(false);
}

wxString NetworkScanDialog::FormatDeviceType(const std::string& deviceType)
{
    if (deviceType == "FluidNC") return "FluidNC Device";
    if (deviceType == "Router") return "Router/Gateway";
    if (deviceType == "Unknown") return "Unknown Device";
    return deviceType;
}

wxString NetworkScanDialog::FormatResponseTime(int responseTime)
{
    if (responseTime < 0) return "-";
    return wxString::Format("%dms", responseTime);
}

wxColour NetworkScanDialog::GetStatusColor(bool isReachable)
{
    return isReachable ? wxColour(0, 128, 0) : wxColour(128, 128, 128);
}

wxString NetworkScanDialog::GetSubnetInfo()
{
    // Get the actual detected subnet from NetworkScanner
    std::string detectedSubnet = NetworkScanner::GetLocalSubnet();
    return wxString::Format("Scanning subnet: %s", detectedSubnet);
}
