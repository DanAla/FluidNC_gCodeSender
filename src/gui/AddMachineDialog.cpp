/**
 * gui/AddMachineDialog.cpp
 * Add Machine Dialog implementation
 */

#include "AddMachineDialog.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/valtext.h>
#include <wx/regex.h>
#include <wx/progdlg.h>
#include <chrono>
#include <future>

// Windows socket includes
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

// Control IDs
enum {
    ID_NAME_CTRL = wxID_HIGHEST + 5000,
    ID_DESCRIPTION_CTRL,
    ID_MACHINE_TYPE_CTRL,
    ID_PROTOCOL_CTRL,
    ID_HOST_CTRL,
    ID_PORT_CTRL,
    ID_SERIAL_PORT_CTRL,
    ID_BAUD_RATE_CTRL,
    ID_AUTO_CONNECT_CTRL,
    ID_ENABLE_LOGGING_CTRL,
    ID_TIMEOUT_CTRL,
    ID_RETRY_COUNT_CTRL,
    ID_TEST_CONNECTION_BTN
};

wxBEGIN_EVENT_TABLE(AddMachineDialog, wxDialog)
    EVT_BUTTON(wxID_OK, AddMachineDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, AddMachineDialog::OnCancel)
    EVT_BUTTON(ID_TEST_CONNECTION_BTN, AddMachineDialog::OnTestConnection)
    EVT_CHOICE(ID_PROTOCOL_CTRL, AddMachineDialog::OnProtocolChange)
    EVT_TEXT(ID_NAME_CTRL, AddMachineDialog::OnValidateInput)
    EVT_TEXT(ID_HOST_CTRL, AddMachineDialog::OnValidateInput)
wxEND_EVENT_TABLE()

AddMachineDialog::AddMachineDialog(wxWindow* parent, bool isEditMode, const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(500, 600),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), m_isEditMode(isEditMode)
{
    CreateControls();
    
    // Set button text based on mode
    if (m_isEditMode) {
        m_okBtn->SetLabel("Save Changes");
    } else {
        m_okBtn->SetLabel("Add Machine");
        
        // Set default values only for add mode
        m_machineTypeCtrl->SetSelection(0); // FluidNC
        m_protocolCtrl->SetSelection(0);    // Telnet
        m_hostCtrl->SetValue("192.168.1.100"); // Default IP for new machines
        m_portCtrl->SetValue(23);           // Default telnet port
        m_baudRateCtrl->SetSelection(5);    // 115200 baud (index 5)
        m_timeoutCtrl->SetValue(5000);      // 5 seconds
        m_retryCountCtrl->SetValue(3);      // 3 retries
    }
    
    UpdateConnectionFields();
    EnableControls();
    
    // Focus on name field
    m_nameCtrl->SetFocus();
}

void AddMachineDialog::CreateControls()
{
    m_mainPanel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    CreateBasicSettings();
    CreateConnectionSettings();
    CreateAdvancedSettings();
    CreateButtonPanel();
    
    mainSizer->Add(m_basicSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(m_connectionSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    mainSizer->Add(m_advancedSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Add stretch spacer
    mainSizer->AddStretchSpacer(1);
    
    // Button panel
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_testBtn = new wxButton(m_mainPanel, ID_TEST_CONNECTION_BTN, "Test Connection");
    btnSizer->Add(m_testBtn, 0, wxRIGHT, 10);
    btnSizer->AddStretchSpacer();
    
    m_okBtn = new wxButton(m_mainPanel, wxID_OK, "Add Machine");
    m_cancelBtn = new wxButton(m_mainPanel, wxID_CANCEL, "Cancel");
    
    btnSizer->Add(m_okBtn, 0, wxRIGHT, 5);
    btnSizer->Add(m_cancelBtn, 0);
    
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 10);
    
    m_mainPanel->SetSizer(mainSizer);
    
    // Main dialog sizer
    wxBoxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);
    dialogSizer->Add(m_mainPanel, 1, wxEXPAND);
    SetSizer(dialogSizer);
}

void AddMachineDialog::CreateBasicSettings()
{
    wxStaticBox* basicBox = new wxStaticBox(m_mainPanel, wxID_ANY, "Basic Settings");
    m_basicSizer = new wxStaticBoxSizer(basicBox, wxVERTICAL);
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(3, 2, 5, 10);
    gridSizer->AddGrowableCol(1, 1);
    
    // Machine Name
    gridSizer->Add(new wxStaticText(m_mainPanel, wxID_ANY, "Machine Name:*"), 0, wxALIGN_CENTER_VERTICAL);
    m_nameCtrl = new wxTextCtrl(m_mainPanel, ID_NAME_CTRL, "", wxDefaultPosition, wxDefaultSize, 0,
                               wxTextValidator(wxFILTER_EMPTY));
    gridSizer->Add(m_nameCtrl, 1, wxEXPAND);
    
    // Description
    gridSizer->Add(new wxStaticText(m_mainPanel, wxID_ANY, "Description:"), 0, wxALIGN_CENTER_VERTICAL);
    m_descriptionCtrl = new wxTextCtrl(m_mainPanel, ID_DESCRIPTION_CTRL, "", wxDefaultPosition, wxDefaultSize,
                                      wxTE_MULTILINE, wxDefaultValidator);
    m_descriptionCtrl->SetMinSize(wxSize(-1, 60));
    gridSizer->Add(m_descriptionCtrl, 1, wxEXPAND);
    
    // Machine Type
    gridSizer->Add(new wxStaticText(m_mainPanel, wxID_ANY, "Machine Type:"), 0, wxALIGN_CENTER_VERTICAL);
    m_machineTypeCtrl = new wxChoice(m_mainPanel, ID_MACHINE_TYPE_CTRL);
    m_machineTypeCtrl->Append("FluidNC");
    m_machineTypeCtrl->Append("GRBL");
    m_machineTypeCtrl->Append("Marlin");
    m_machineTypeCtrl->Append("LinuxCNC");
    m_machineTypeCtrl->Append("Other");
    gridSizer->Add(m_machineTypeCtrl, 1, wxEXPAND);
    
    m_basicSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);
}

void AddMachineDialog::CreateConnectionSettings()
{
    wxStaticBox* connBox = new wxStaticBox(m_mainPanel, wxID_ANY, "Connection Settings");
    m_connectionSizer = new wxStaticBoxSizer(connBox, wxVERTICAL);
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(5, 2, 5, 10);
    gridSizer->AddGrowableCol(1, 1);
    
    // Protocol
    gridSizer->Add(new wxStaticText(m_mainPanel, wxID_ANY, "Protocol:"), 0, wxALIGN_CENTER_VERTICAL);
    m_protocolCtrl = new wxChoice(m_mainPanel, ID_PROTOCOL_CTRL);
    m_protocolCtrl->Append("Telnet");
    m_protocolCtrl->Append("USB/Serial");
    m_protocolCtrl->Append("WebSocket");
    gridSizer->Add(m_protocolCtrl, 1, wxEXPAND);
    
    // Host/IP (for network connections)
    m_hostLabel = new wxStaticText(m_mainPanel, wxID_ANY, "Host/IP Address:*");
    gridSizer->Add(m_hostLabel, 0, wxALIGN_CENTER_VERTICAL);
    m_hostCtrl = new wxTextCtrl(m_mainPanel, ID_HOST_CTRL, "");
    gridSizer->Add(m_hostCtrl, 1, wxEXPAND);
    
    // Port (for network connections)
    m_portLabel = new wxStaticText(m_mainPanel, wxID_ANY, "Port:");
    gridSizer->Add(m_portLabel, 0, wxALIGN_CENTER_VERTICAL);
    m_portCtrl = new wxSpinCtrl(m_mainPanel, ID_PORT_CTRL, "", wxDefaultPosition, wxDefaultSize,
                               wxSP_ARROW_KEYS, 1, 65535, 23);
    gridSizer->Add(m_portCtrl, 1, wxEXPAND);
    
    // Serial Port (for USB/Serial connections)
    m_serialPortLabel = new wxStaticText(m_mainPanel, wxID_ANY, "Serial Port:");
    gridSizer->Add(m_serialPortLabel, 0, wxALIGN_CENTER_VERTICAL);
    m_serialPortCtrl = new wxChoice(m_mainPanel, ID_SERIAL_PORT_CTRL);
    m_serialPortCtrl->Append("COM1");
    m_serialPortCtrl->Append("COM2");
    m_serialPortCtrl->Append("COM3");
    m_serialPortCtrl->Append("COM4");
    m_serialPortCtrl->Append("COM5");
    m_serialPortCtrl->Append("COM6");
    m_serialPortCtrl->Append("COM7");
    m_serialPortCtrl->Append("COM8");
    gridSizer->Add(m_serialPortCtrl, 1, wxEXPAND);
    
    // Baud Rate (for USB/Serial connections)
    m_baudRateLabel = new wxStaticText(m_mainPanel, wxID_ANY, "Baud Rate:");
    gridSizer->Add(m_baudRateLabel, 0, wxALIGN_CENTER_VERTICAL);
    m_baudRateCtrl = new wxChoice(m_mainPanel, ID_BAUD_RATE_CTRL);
    m_baudRateCtrl->Append("9600");
    m_baudRateCtrl->Append("19200");
    m_baudRateCtrl->Append("38400");
    m_baudRateCtrl->Append("57600");
    m_baudRateCtrl->Append("74880");
    m_baudRateCtrl->Append("115200");
    m_baudRateCtrl->Append("230400");
    m_baudRateCtrl->Append("250000");
    gridSizer->Add(m_baudRateCtrl, 1, wxEXPAND);
    
    m_connectionSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);
}

void AddMachineDialog::CreateAdvancedSettings()
{
    wxStaticBox* advBox = new wxStaticBox(m_mainPanel, wxID_ANY, "Advanced Settings");
    m_advancedSizer = new wxStaticBoxSizer(advBox, wxVERTICAL);
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(4, 2, 5, 10);
    gridSizer->AddGrowableCol(1, 1);
    
    // Auto Connect
    m_autoConnectCtrl = new wxCheckBox(m_mainPanel, ID_AUTO_CONNECT_CTRL, "Auto-connect on startup");
    gridSizer->Add(new wxStaticText(m_mainPanel, wxID_ANY, ""), 0); // Empty cell
    gridSizer->Add(m_autoConnectCtrl, 1, wxEXPAND);
    
    // Enable Logging
    m_enableLoggingCtrl = new wxCheckBox(m_mainPanel, ID_ENABLE_LOGGING_CTRL, "Enable connection logging");
    gridSizer->Add(new wxStaticText(m_mainPanel, wxID_ANY, ""), 0); // Empty cell
    gridSizer->Add(m_enableLoggingCtrl, 1, wxEXPAND);
    m_enableLoggingCtrl->SetValue(true); // Default enabled
    
    // Connection Timeout
    gridSizer->Add(new wxStaticText(m_mainPanel, wxID_ANY, "Connection Timeout (ms):"), 0, wxALIGN_CENTER_VERTICAL);
    m_timeoutCtrl = new wxSpinCtrl(m_mainPanel, ID_TIMEOUT_CTRL, "", wxDefaultPosition, wxDefaultSize,
                                  wxSP_ARROW_KEYS, 1000, 30000, 5000);
    gridSizer->Add(m_timeoutCtrl, 1, wxEXPAND);
    
    // Retry Count
    gridSizer->Add(new wxStaticText(m_mainPanel, wxID_ANY, "Connection Retries:"), 0, wxALIGN_CENTER_VERTICAL);
    m_retryCountCtrl = new wxSpinCtrl(m_mainPanel, ID_RETRY_COUNT_CTRL, "", wxDefaultPosition, wxDefaultSize,
                                     wxSP_ARROW_KEYS, 0, 10, 3);
    gridSizer->Add(m_retryCountCtrl, 1, wxEXPAND);
    
    m_advancedSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);
}

void AddMachineDialog::CreateButtonPanel()
{
    // Button panel creation is handled in CreateControls()
}

AddMachineDialog::MachineData AddMachineDialog::GetMachineData() const
{
    MachineData data;
    data.name = m_nameCtrl->GetValue();
    data.description = m_descriptionCtrl->GetValue();
    data.host = m_hostCtrl->GetValue();
    data.port = m_portCtrl->GetValue();
    data.protocol = m_protocolCtrl->GetStringSelection();
    data.machineType = m_machineTypeCtrl->GetStringSelection();
    data.baudRate = m_baudRateCtrl->GetStringSelection();
    data.serialPort = m_serialPortCtrl->GetStringSelection();
    return data;
}

void AddMachineDialog::SetMachineData(const MachineData& data)
{
    m_nameCtrl->SetValue(data.name);
    m_descriptionCtrl->SetValue(data.description);
    m_hostCtrl->SetValue(data.host);
    m_portCtrl->SetValue(data.port);
    
    // Set selections
    m_protocolCtrl->SetStringSelection(data.protocol);
    m_machineTypeCtrl->SetStringSelection(data.machineType);
    m_baudRateCtrl->SetStringSelection(data.baudRate);
    m_serialPortCtrl->SetStringSelection(data.serialPort);
    
    UpdateConnectionFields();
}

bool AddMachineDialog::ValidateInput()
{
    // Check required fields
    if (m_nameCtrl->GetValue().Trim().IsEmpty()) {
        wxMessageBox("Please enter a machine name.", "Validation Error", wxOK | wxICON_WARNING, this);
        m_nameCtrl->SetFocus();
        return false;
    }
    
    // Check for duplicate names (simplified - would check against existing machines in real implementation)
    wxString name = m_nameCtrl->GetValue().Trim();
    if (name.Lower() == "default" || name.Lower() == "new machine") {
        wxMessageBox("Please choose a different machine name.", "Validation Error", wxOK | wxICON_WARNING, this);
        m_nameCtrl->SetFocus();
        return false;
    }
    
    // Validate connection settings based on protocol
    wxString protocol = m_protocolCtrl->GetStringSelection();
    if (protocol == "Telnet" || protocol == "WebSocket") {
        if (m_hostCtrl->GetValue().Trim().IsEmpty()) {
            wxMessageBox("Please enter a host/IP address for network connections.", "Validation Error", wxOK | wxICON_WARNING, this);
            m_hostCtrl->SetFocus();
            return false;
        }
        
        // Basic IP address validation (simplified)
        wxString host = m_hostCtrl->GetValue().Trim();
        if (!host.Contains(".") && host != "localhost") {
            wxMessageBox("Please enter a valid IP address or hostname.", "Validation Error", wxOK | wxICON_WARNING, this);
            m_hostCtrl->SetFocus();
            return false;
        }
    } else if (protocol == "USB/Serial") {
        if (m_serialPortCtrl->GetSelection() == wxNOT_FOUND) {
            wxMessageBox("Please select a serial port for USB/Serial connections.", "Validation Error", wxOK | wxICON_WARNING, this);
            return false;
        }
    }
    
    // Validate machine type selection
    if (m_machineTypeCtrl->GetSelection() == wxNOT_FOUND) {
        wxMessageBox("Please select a machine type.", "Validation Error", wxOK | wxICON_WARNING, this);
        return false;
    }
    
    return true;
}

void AddMachineDialog::UpdateConnectionFields()
{
    wxString protocol = m_protocolCtrl->GetStringSelection();
    
    bool isNetworkProtocol = (protocol == "Telnet" || protocol == "WebSocket");
    bool isSerialProtocol = (protocol == "USB/Serial");
    
    // Show/hide network fields
    m_hostLabel->Show(isNetworkProtocol);
    m_hostCtrl->Show(isNetworkProtocol);
    m_portLabel->Show(isNetworkProtocol);
    m_portCtrl->Show(isNetworkProtocol);
    
    // Show/hide serial fields
    m_serialPortLabel->Show(isSerialProtocol);
    m_serialPortCtrl->Show(isSerialProtocol);
    m_baudRateLabel->Show(isSerialProtocol);
    m_baudRateCtrl->Show(isSerialProtocol);
    
    // Update port defaults
    if (protocol == "Telnet") {
        m_portCtrl->SetValue(23);
    } else if (protocol == "WebSocket") {
        m_portCtrl->SetValue(80);
    }
    
    m_mainPanel->Layout();
    Fit();
}

void AddMachineDialog::EnableControls()
{
    bool hasName = !m_nameCtrl->GetValue().Trim().IsEmpty();
    bool hasHost = !m_hostCtrl->GetValue().Trim().IsEmpty();
    bool isSerial = m_protocolCtrl->GetStringSelection() == "USB/Serial";
    
    bool canTest = hasName && (hasHost || isSerial);
    m_testBtn->Enable(canTest);
    m_okBtn->Enable(hasName);
}

// Event handlers
void AddMachineDialog::OnOK(wxCommandEvent& WXUNUSED(event))
{
    if (ValidateInput()) {
        EndModal(wxID_OK);
    }
}

void AddMachineDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_CANCEL);
}

void AddMachineDialog::OnProtocolChange(wxCommandEvent& WXUNUSED(event))
{
    UpdateConnectionFields();
    EnableControls();
}

void AddMachineDialog::OnTestConnection(wxCommandEvent& WXUNUSED(event))
{
    if (!ValidateInput()) {
        return;
    }
    
    MachineData data = GetMachineData();
    
    // Only test network protocols for now
    if (data.protocol != "Telnet" && data.protocol != "WebSocket") {
        wxMessageBox(
            wxString::Format("Connection testing for %s protocol is not yet implemented.\n\n"
                           "This feature will be added in a future update.",
                           data.protocol),
            "Connection Test - Not Implemented", wxOK | wxICON_INFORMATION, this);
        return;
    }
    
    // Show progress dialog
    wxProgressDialog* progressDlg = new wxProgressDialog(
        "Testing Connection",
        wxString::Format("Testing connection to %s:%d...", data.host, data.port),
        100, this,
        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT
    );
    
    progressDlg->Pulse("Connecting...");
    
    // Test connection in a separate thread to avoid blocking UI
    std::future<bool> connectionTest = std::async(std::launch::async, [this, &data]() {
        return TestTelnetConnection(data.host.ToStdString(), data.port);
    });
    
    // Wait for connection test with timeout
    auto status = connectionTest.wait_for(std::chrono::seconds(5));
    
    progressDlg->Destroy();
    
    if (status == std::future_status::timeout) {
        wxMessageBox(
            wxString::Format("Connection test to '%s' timed out.\n\n"
                           "Host: %s\n"
                           "Port: %d\n\n"
                           "The machine may be offline or unreachable.",
                           data.name, data.host, data.port),
            "Connection Test - Timeout", wxOK | wxICON_WARNING, this);
    } else {
        bool success = connectionTest.get();
        if (success) {
            wxMessageBox(
                wxString::Format("Connection test to '%s' was successful!\n\n"
                               "Host: %s\n"
                               "Port: %d\n\n"
                               "The machine is reachable and accepting connections.",
                               data.name, data.host, data.port),
                "Connection Test - Success", wxOK | wxICON_INFORMATION, this);
        } else {
            wxMessageBox(
                wxString::Format("Connection test to '%s' failed.\n\n"
                               "Host: %s\n"
                               "Port: %d\n\n"
                               "Please check that:\n"
                               "- The machine is powered on and connected\n"
                               "- The network connection is working\n"
                               "- The host address and port are correct\n"
                               "- No firewall is blocking the connection",
                               data.name, data.host, data.port),
                "Connection Test - Failed", wxOK | wxICON_ERROR, this);
        }
    }
}

void AddMachineDialog::OnValidateInput(wxCommandEvent& WXUNUSED(event))
{
    EnableControls();
}

// Connection helper methods
bool AddMachineDialog::TestTelnetConnection(const std::string& host, int port)
{
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        return false;
    }
    
    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }
    
    // Set socket to non-blocking mode for timeout control
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
    
    // Set up address
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    // Convert hostname to IP address
    struct addrinfo hints, *res;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) {
        closesocket(sock);
        WSACleanup();
        return false;
    }
    
    addr = *((sockaddr_in*)res->ai_addr);
    freeaddrinfo(res);
    
    // Attempt to connect
    int connectResult = connect(sock, (sockaddr*)&addr, sizeof(addr));
    
    if (connectResult == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            // Connection in progress, wait for completion
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(sock, &writeSet);
            
            struct timeval timeout;
            timeout.tv_sec = 3;  // 3 second timeout
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
        closesocket(sock);
        WSACleanup();
        return false;
    }
    
    // Connection successful
    closesocket(sock);
    WSACleanup();
    return true;
#else
    // Unix/Linux implementation would go here
    return false;
#endif
}
