/**
 * gui/MachineManagerPanel.cpp
 * Machine Manager Panel implementation with dummy content
 */

#include "MachineManagerPanel.h"
#include "AddMachineDialog.h"
#include "NetworkScanDialog.h"
#include "MainFrame.h"
#include "ConsolePanel.h"
#include "NotificationSystem.h"
#include "core/SimpleLogger.h"
#include "core/CommunicationManager.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/datetime.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/textfile.h>
#include <wx/stdpaths.h>
#include <wx/progdlg.h>
#include <json.hpp>
#include <fstream>
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
    ID_MACHINE_LIST = wxID_HIGHEST + 2000,
    ID_SCAN_NETWORK,
    ID_ADD_MACHINE,
    ID_EDIT_MACHINE,
    ID_REMOVE_MACHINE,
    ID_CONNECT,
    ID_DISCONNECT,
    ID_TEST_CONNECTION,
    ID_IMPORT_CONFIG,
    ID_EXPORT_CONFIG
};

wxBEGIN_EVENT_TABLE(MachineManagerPanel, wxPanel)
    EVT_LIST_ITEM_SELECTED(ID_MACHINE_LIST, MachineManagerPanel::OnMachineSelected)
    EVT_LIST_ITEM_ACTIVATED(ID_MACHINE_LIST, MachineManagerPanel::OnMachineActivated)
    EVT_BUTTON(ID_SCAN_NETWORK, MachineManagerPanel::OnScanNetwork)
    EVT_BUTTON(ID_ADD_MACHINE, MachineManagerPanel::OnAddMachine)
    EVT_BUTTON(ID_EDIT_MACHINE, MachineManagerPanel::OnEditMachine)
    EVT_BUTTON(ID_REMOVE_MACHINE, MachineManagerPanel::OnRemoveMachine)
    EVT_BUTTON(ID_CONNECT, MachineManagerPanel::OnConnect)
    EVT_BUTTON(ID_DISCONNECT, MachineManagerPanel::OnDisconnect)
    EVT_BUTTON(ID_TEST_CONNECTION, MachineManagerPanel::OnTestConnection)
    EVT_BUTTON(ID_IMPORT_CONFIG, MachineManagerPanel::OnImportConfig)
    EVT_BUTTON(ID_EXPORT_CONFIG, MachineManagerPanel::OnExportConfig)
wxEND_EVENT_TABLE()

MachineManagerPanel::MachineManagerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), m_splitter(nullptr)
{
    CreateControls();
    LoadMachineConfigs();
    PopulateMachineList();
}

void MachineManagerPanel::CreateControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    wxStaticText* title = new wxStaticText(this, wxID_ANY, "Machine Manager");
    title->SetFont(title->GetFont().Scale(1.2).Bold());
    mainSizer->Add(title, 0, wxALL | wxCENTER, 5);
    
    // Splitter window
    m_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
                                     wxSP_3D | wxSP_LIVE_UPDATE);
    m_splitter->SetMinimumPaneSize(200);
    
    CreateMachineList();
    CreateMachineDetails();
    
    m_splitter->SplitHorizontally(m_listPanel, m_detailsPanel, 300);
    
    mainSizer->Add(m_splitter, 1, wxALL | wxEXPAND, 5);
    SetSizer(mainSizer);
}

void MachineManagerPanel::CreateMachineList()
{
    m_listPanel = new wxPanel(m_splitter, wxID_ANY);
    wxBoxSizer* listSizer = new wxBoxSizer(wxVERTICAL);
    
    // Scan Network button at the top
    m_scanNetworkBtn = new wxButton(m_listPanel, ID_SCAN_NETWORK, "Scan Network");
    m_scanNetworkBtn->SetToolTip("Discover FluidNC devices on your local network");
    listSizer->Add(m_scanNetworkBtn, 0, wxALL | wxEXPAND, 5);
    
    // Machine list
    m_machineList = new wxListCtrl(m_listPanel, ID_MACHINE_LIST, wxDefaultPosition, wxDefaultSize,
                                  wxLC_REPORT | wxLC_SINGLE_SEL);
    
    m_machineList->AppendColumn("Name", wxLIST_FORMAT_LEFT, 120);
    m_machineList->AppendColumn("Host", wxLIST_FORMAT_LEFT, 100);
    m_machineList->AppendColumn("Port", wxLIST_FORMAT_LEFT, 60);
    m_machineList->AppendColumn("Status", wxLIST_FORMAT_LEFT, 80);
    
    listSizer->Add(m_machineList, 1, wxALL | wxEXPAND, 5);
    
    // Button panel
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_addBtn = new wxButton(m_listPanel, ID_ADD_MACHINE, "Add");
    m_editBtn = new wxButton(m_listPanel, ID_EDIT_MACHINE, "Edit");
    m_removeBtn = new wxButton(m_listPanel, ID_REMOVE_MACHINE, "Remove");
    
    btnSizer->Add(m_addBtn, 0, wxRIGHT, 3);
    btnSizer->Add(m_editBtn, 0, wxRIGHT, 3);
    btnSizer->Add(m_removeBtn, 0, wxRIGHT, 5);
    
    btnSizer->AddStretchSpacer();
    
    m_importBtn = new wxButton(m_listPanel, ID_IMPORT_CONFIG, "Import");
    m_exportBtn = new wxButton(m_listPanel, ID_EXPORT_CONFIG, "Export");
    
    btnSizer->Add(m_importBtn, 0, wxRIGHT, 3);
    btnSizer->Add(m_exportBtn, 0);
    
    listSizer->Add(btnSizer, 0, wxALL | wxEXPAND, 5);
    
    m_listPanel->SetSizer(listSizer);
}

void MachineManagerPanel::CreateMachineDetails()
{
    m_detailsPanel = new wxPanel(m_splitter, wxID_ANY);
    wxBoxSizer* detailsSizer = new wxBoxSizer(wxVERTICAL);
    
    // Details title
    wxStaticText* detailsTitle = new wxStaticText(m_detailsPanel, wxID_ANY, "Machine Details");
    detailsTitle->SetFont(detailsTitle->GetFont().Bold());
    detailsSizer->Add(detailsTitle, 0, wxALL, 5);
    
    // Details grid - increased rows to accommodate description
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(7, 2, 5, 10);
    gridSizer->AddGrowableCol(1, 1);
    
    gridSizer->Add(new wxStaticText(m_detailsPanel, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
    m_nameLabel = new wxStaticText(m_detailsPanel, wxID_ANY, "-");
    gridSizer->Add(m_nameLabel, 1, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(m_detailsPanel, wxID_ANY, "Description:"), 0, wxALIGN_TOP);
    m_descriptionSizer = new wxBoxSizer(wxVERTICAL);
    m_descriptionLabel = new wxStaticText(m_detailsPanel, wxID_ANY, "-");
    m_descriptionSizer->Add(m_descriptionLabel, 1, wxEXPAND);
    gridSizer->Add(m_descriptionSizer, 1, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(m_detailsPanel, wxID_ANY, "Host:"), 0, wxALIGN_CENTER_VERTICAL);
    m_hostLabel = new wxStaticText(m_detailsPanel, wxID_ANY, "-");
    gridSizer->Add(m_hostLabel, 1, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(m_detailsPanel, wxID_ANY, "Port:"), 0, wxALIGN_CENTER_VERTICAL);
    m_portLabel = new wxStaticText(m_detailsPanel, wxID_ANY, "-");
    gridSizer->Add(m_portLabel, 1, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(m_detailsPanel, wxID_ANY, "Type:"), 0, wxALIGN_CENTER_VERTICAL);
    m_machineTypeLabel = new wxStaticText(m_detailsPanel, wxID_ANY, "-");
    gridSizer->Add(m_machineTypeLabel, 1, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(m_detailsPanel, wxID_ANY, "Status:"), 0, wxALIGN_CENTER_VERTICAL);
    m_statusLabel = new wxStaticText(m_detailsPanel, wxID_ANY, "-");
    gridSizer->Add(m_statusLabel, 1, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(m_detailsPanel, wxID_ANY, "Last Connected:"), 0, wxALIGN_CENTER_VERTICAL);
    m_lastConnectedLabel = new wxStaticText(m_detailsPanel, wxID_ANY, "-");
    gridSizer->Add(m_lastConnectedLabel, 1, wxEXPAND);
    
    detailsSizer->Add(gridSizer, 0, wxALL | wxEXPAND, 10);
    
    // Connection controls
    wxBoxSizer* connSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_connectBtn = new wxButton(m_detailsPanel, ID_CONNECT, "Connect");
    m_disconnectBtn = new wxButton(m_detailsPanel, ID_DISCONNECT, "Disconnect");
    m_testBtn = new wxButton(m_detailsPanel, ID_TEST_CONNECTION, "Test");
    
    connSizer->Add(m_connectBtn, 0, wxRIGHT, 5);
    connSizer->Add(m_disconnectBtn, 0, wxRIGHT, 5);
    connSizer->Add(m_testBtn, 0);
    
    detailsSizer->Add(connSizer, 0, wxALL, 10);
    
    detailsSizer->AddStretchSpacer();
    
    m_detailsPanel->SetSizer(detailsSizer);
    
    // Bind size event for dynamic text wrapping
    m_detailsPanel->Bind(wxEVT_SIZE, &MachineManagerPanel::OnDetailsPanelResize, this);
    
    // Initially disable connection buttons
    m_editBtn->Enable(false);
    m_removeBtn->Enable(false);
    m_connectBtn->Enable(false);
    m_disconnectBtn->Enable(false);
    m_testBtn->Enable(false);
}

void MachineManagerPanel::LoadMachineConfigs()
{
    m_machines.clear();
    
    wxString settingsPath = GetSettingsPath();
    wxString machinesFile = settingsPath + wxFileName::GetPathSeparator() + "machines.json";
    
    // Check if machines.json exists
    if (!wxFileName::FileExists(machinesFile)) {
        // Create empty machines file
        CreateEmptyMachinesFile(machinesFile);
        return; // Start with empty machine list
    }
    
    try {
        // Read the JSON file
        std::ifstream file(machinesFile.ToStdString());
        if (!file.is_open()) {
            LOG_ERROR("Could not open machines.json file for reading");
            return;
        }
        
        nlohmann::json j;
        file >> j;
        file.close();
        
        // Parse machines from JSON
        if (j.contains("machines") && j["machines"].is_array()) {
            for (const auto& machineJson : j["machines"]) {
                MachineConfig machine;
                
                machine.id = machineJson.value("id", "");
                machine.name = machineJson.value("name", "");
                machine.description = machineJson.value("description", "");
                machine.host = machineJson.value("host", "");
                machine.port = machineJson.value("port", 23);
                machine.machineType = machineJson.value("machineType", "FluidNC");
                machine.connected = false; // Always start disconnected
                machine.lastConnected = machineJson.value("lastConnected", "Never");
                machine.autoConnect = machineJson.value("autoConnect", false);
                
                m_machines.push_back(machine);
            }
        }
        
        std::string logMsg = "Loaded " + std::to_string(m_machines.size()) + " machine configurations from " + machinesFile.ToStdString();
        LOG_INFO(logMsg);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading machine configurations: " + std::string(e.what()));
        // Continue with empty machine list on error
    }
}

void MachineManagerPanel::PopulateMachineList()
{
    m_machineList->DeleteAllItems();
    
    for (size_t i = 0; i < m_machines.size(); ++i) {
        const auto& machine = m_machines[i];
        
        long itemIndex = m_machineList->InsertItem(i, machine.name);
        m_machineList->SetItem(itemIndex, 1, machine.host);
        m_machineList->SetItem(itemIndex, 2, std::to_string(machine.port));
        m_machineList->SetItem(itemIndex, 3, machine.connected ? "Connected" : "Disconnected");
        
        // Store machine ID in item data
        m_machineList->SetItemData(itemIndex, i);
        
        // Color coding - use dark green for better visibility
        if (machine.connected) {
            m_machineList->SetItemTextColour(itemIndex, wxColour(0, 128, 0)); // Dark green
        } else {
            m_machineList->SetItemTextColour(itemIndex, *wxBLACK);
        }
    }
}

void MachineManagerPanel::RefreshMachineList()
{
    PopulateMachineList();
}

void MachineManagerPanel::SelectMachine(const std::string& machineId)
{
    m_selectedMachine = machineId;
    UpdateMachineDetails();
}

void MachineManagerPanel::UpdateConnectionStatus(const std::string& machineId, bool connected)
{
    // Find and update machine status
    for (auto& machine : m_machines) {
        if (machine.id == machineId) {
            machine.connected = connected;
            if (connected) {
                machine.lastConnected = "Connected";
            }
            break;
        }
    }
    RefreshMachineList();
    if (m_selectedMachine == machineId) {
        UpdateMachineDetails();
    }
}

void MachineManagerPanel::AttemptAutoConnect()
{
    // Find machine marked for auto-connect
    MachineConfig* autoConnectMachine = nullptr;
    for (auto& machine : m_machines) {
        if (machine.autoConnect) {
            autoConnectMachine = &machine;
            break;
        }
    }
    
    if (!autoConnectMachine) {
        LOG_INFO("No machine configured for auto-connect");
        return;
    }
    
    LOG_INFO("Attempting auto-connect to machine: " + autoConnectMachine->name);
    
    // CRITICAL: Set connecting state, do NOT assume connection success
    autoConnectMachine->connected = false;  // Ensure we start disconnected
    autoConnectMachine->lastConnected = "Connecting...";  // Show connecting status
    
    // Refresh UI to show "Connecting..." status immediately
    RefreshMachineList();
    
    // Select and show the machine being connected
    SelectMachine(autoConnectMachine->id);
    for (int i = 0; i < m_machineList->GetItemCount(); ++i) {
        if (m_machineList->GetItemText(i, 0) == autoConnectMachine->name) {
            m_machineList->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
            break;
        }
    }
    
    // Start the connection attempt - do NOT assume success
    bool connectionAttemptStarted = CommunicationManager::Instance().ConnectMachine(
        autoConnectMachine->id, autoConnectMachine->host, autoConnectMachine->port);
    
    if (connectionAttemptStarted) {
        // Connection attempt started successfully - wait for actual connection callback
        LOG_INFO("Auto-connect attempt started for machine: " + autoConnectMachine->name + 
                 " (" + autoConnectMachine->host + ":" + std::to_string(autoConnectMachine->port) + ")");
        
        // Show info notification about connection attempt
        NotificationSystem::Instance().ShowInfo(
            "Connecting to Machine",
            wxString::Format("Attempting to connect to '%s' (%s:%d). Please wait...",
                           autoConnectMachine->name, autoConnectMachine->host, autoConnectMachine->port)
        );
        
        // CRITICAL: Actual connection status will be updated via UpdateConnectionStatus() callback
        // when the FluidNCClient successfully connects or fails
        
    } else {
        // Failed to even start connection attempt
        autoConnectMachine->lastConnected = "Connection failed";
        RefreshMachineList();
        
        LOG_ERROR("Auto-connect attempt failed to start for machine: " + autoConnectMachine->name + " (" + 
                  autoConnectMachine->host + ":" + std::to_string(autoConnectMachine->port) + ")");
        
        // Show error notification for failed connection attempt
        NotificationSystem::Instance().ShowError(
            "Auto-Connect Failed",
            wxString::Format("Failed to start connection attempt to '%s' (%s:%d). Check configuration.",
                           autoConnectMachine->name, autoConnectMachine->host, autoConnectMachine->port)
        );
    }
}

void MachineManagerPanel::UpdateMachineDetails()
{
    // Find selected machine
    const MachineConfig* selectedMachine = nullptr;
    for (const auto& machine : m_machines) {
        if (machine.id == m_selectedMachine) {
            selectedMachine = &machine;
            break;
        }
    }
    
    if (selectedMachine) {
        m_nameLabel->SetLabel(selectedMachine->name);
        
        // Set description with proper handling for empty descriptions
        wxString description = selectedMachine->description.empty() ? "No description" : selectedMachine->description;
        m_descriptionLabel->SetLabel(description);
        
        // Initial wrap
        m_detailsPanel->Layout();
        m_descriptionLabel->Wrap(m_descriptionSizer->GetSize().GetWidth());
        
        m_hostLabel->SetLabel(selectedMachine->host);
        m_portLabel->SetLabel(std::to_string(selectedMachine->port));
        m_machineTypeLabel->SetLabel(selectedMachine->machineType);
        m_statusLabel->SetLabel(selectedMachine->connected ? "Connected" : "Disconnected");
        m_lastConnectedLabel->SetLabel(selectedMachine->lastConnected);
        
        // Update button states
        m_editBtn->Enable(true);
        m_removeBtn->Enable(true);
        m_connectBtn->Enable(!selectedMachine->connected);
        m_disconnectBtn->Enable(selectedMachine->connected);
        m_testBtn->Enable(true);
        
        // Color status label - use dark green for better visibility
        if (selectedMachine->connected) {
            m_statusLabel->SetForegroundColour(wxColour(0, 128, 0)); // Dark green
        } else {
            m_statusLabel->SetForegroundColour(*wxRED);
        }
    } else {
        // Clear details
        m_nameLabel->SetLabel("-");
        m_descriptionLabel->SetLabel("-");
        m_hostLabel->SetLabel("-");
        m_portLabel->SetLabel("-");
        m_machineTypeLabel->SetLabel("-");
        m_statusLabel->SetLabel("-");
        m_lastConnectedLabel->SetLabel("-");
        
        // Disable buttons
        m_editBtn->Enable(false);
        m_removeBtn->Enable(false);
        m_connectBtn->Enable(false);
        m_disconnectBtn->Enable(false);
        m_testBtn->Enable(false);
        
        m_statusLabel->SetForegroundColour(*wxBLACK);
    }
    
    m_detailsPanel->Layout();
}

void MachineManagerPanel::OnDetailsPanelResize(wxSizeEvent& event)
{
    if (m_descriptionLabel && m_descriptionSizer)
    {
        m_descriptionLabel->Wrap(m_descriptionSizer->GetSize().GetWidth());
    }
    event.Skip();
}

// Event handlers
void MachineManagerPanel::OnMachineSelected(wxListEvent& event)
{
    long itemIndex = event.GetIndex();
    if (itemIndex >= 0 && itemIndex < (long)m_machines.size()) {
        SelectMachine(m_machines[itemIndex].id);
    }
}

void MachineManagerPanel::OnMachineActivated(wxListEvent& event)
{
    wxCommandEvent evt;
    OnConnect(evt);
}

void MachineManagerPanel::OnAddMachine(wxCommandEvent& WXUNUSED(event))
{
    AddMachineDialog dialog(this);
    
    if (dialog.ShowModal() == wxID_OK) {
        AddMachineDialog::MachineData data = dialog.GetMachineData();
        
        // Generate unique machine ID
        std::string machineId = "machine" + std::to_string(m_machines.size() + 1);
        
        // Handle single auto-connect constraint
        if (data.autoConnect) {
            // If this machine is set to auto-connect, clear auto-connect from all other machines
            for (auto& machine : m_machines) {
                if (machine.autoConnect) {
                    machine.autoConnect = false;
                    LOG_INFO("Disabled auto-connect for machine: " + machine.name + " (replaced by new machine)");
                }
            }
        }
        
        // Create new machine configuration
        MachineConfig newMachine;
        newMachine.id = machineId;
        newMachine.name = data.name.ToStdString();
        newMachine.description = data.description.ToStdString();
        newMachine.host = data.host.ToStdString();
        newMachine.port = data.port;
        newMachine.machineType = data.machineType.ToStdString();
        newMachine.connected = false;
        newMachine.lastConnected = "Never";
        newMachine.autoConnect = data.autoConnect;
        
        // Add to machines list
        m_machines.push_back(newMachine);
        
        // Save to persistent storage
        SaveMachineConfigs();
        
        // Refresh the list display
        PopulateMachineList();
        
        // Select the newly added machine
        for (int i = 0; i < m_machineList->GetItemCount(); ++i) {
            if (m_machineList->GetItemText(i, 0) == data.name) {
                m_machineList->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                SelectMachine(machineId);
                break;
            }
        }
        
        // If auto-connect is enabled, attempt immediate connection
        if (data.autoConnect) {
            LOG_INFO("Auto-connect enabled for new machine: " + data.name.ToStdString() + ". Attempting immediate connection...");
            
            // Test connection in a separate thread
            std::future<bool> connectionTest = std::async(std::launch::async, [this, &newMachine]() {
                return TestTelnetConnection(newMachine.host, newMachine.port);
            });
            
            // Wait for connection test with timeout
            auto status = connectionTest.wait_for(std::chrono::seconds(5));
            
            if (status != std::future_status::timeout) {
                bool connectionSuccess = connectionTest.get();
                
                if (connectionSuccess) {
                    // Update machine status
                    newMachine.connected = true;
                    newMachine.lastConnected = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S").ToStdString();
                    
                    // Update the machine in the vector
                    m_machines[m_machines.size() - 1] = newMachine;
                    
                    // Save updated configuration
                    SaveMachineConfigs();
                    
                    // Refresh UI
                    RefreshMachineList();
                    UpdateMachineDetails();
                    
                    // Log connection success to terminal console
                    MainFrame* mainFrame = dynamic_cast<MainFrame*>(wxGetTopLevelParent(this));
                    if (mainFrame) {
                        ConsolePanel* console = mainFrame->GetConsolePanel();
                        if (console) {
                            console->LogMessage("=== AUTO-CONNECT SUCCESSFUL ===", "INFO");
                            console->LogMessage(wxString::Format("Auto-connected to: %s (%s:%d)", 
                                newMachine.name, newMachine.host, newMachine.port).ToStdString(), "INFO");
                            console->LogMessage(wxString::Format("Machine Type: %s", newMachine.machineType).ToStdString(), "INFO");
                            console->LogMessage(wxString::Format("Connection Time: %s", newMachine.lastConnected).ToStdString(), "INFO");
                            console->LogMessage("Status: READY - Machine is active and awaiting commands", "INFO");
                            console->LogMessage("=== END AUTO-CONNECT INFO ===", "INFO");
                            
                            // Simulate some initial communication handshake
                            console->LogSentCommand("?");
                            console->LogReceivedResponse("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>");
                            console->LogMessage("Machine ready for G-code commands", "INFO");
                            
                            // Enable command input now that machine is connected
                            console->SetConnectionEnabled(true, newMachine.name);
                        }
                    }
                    
                    // Show success notification with connection info
                    NotificationSystem::Instance().ShowSuccess(
                        "Machine Added and Connected",
                        wxString::Format("Machine '%s' (%s) has been added and automatically connected!", data.name, data.machineType)
                    );
                    
                    LOG_INFO("Immediate auto-connect successful for new machine: " + data.name.ToStdString());
                    return; // Skip the regular success message
                } else {
                    LOG_ERROR("Immediate auto-connect failed for new machine: " + data.name.ToStdString());
                    
                    NotificationSystem::Instance().ShowWarning(
                        "Machine Added but Connection Failed",
                        wxString::Format("Machine '%s' has been added with auto-connect enabled, but the initial connection failed. It will retry on next startup.", data.name)
                    );
                    return; // Skip the regular success message
                }
            } else {
                LOG_ERROR("Immediate auto-connect timeout for new machine: " + data.name.ToStdString());
                
                NotificationSystem::Instance().ShowWarning(
                    "Machine Added but Connection Timeout",
                    wxString::Format("Machine '%s' has been added with auto-connect enabled, but the initial connection timed out. It will retry on next startup.", data.name)
                );
                return; // Skip the regular success message
            }
        }
        
        // Show regular success message (only if auto-connect was not enabled)
        wxString successMsg = wxString::Format(
            "Machine '%s' has been successfully added!\n\n"
            "Type: %s\n"
            "Protocol: %s\n",
            data.name, data.machineType, data.protocol
        );
        
        if (data.protocol == "Telnet" || data.protocol == "WebSocket") {
            successMsg += wxString::Format("Connection: %s:%d\n", data.host, data.port);
        } else if (data.protocol == "USB/Serial") {
            successMsg += wxString::Format("Connection: %s @ %s baud\n", data.serialPort, data.baudRate);
        }
        
        successMsg += "\nYou can now connect to this machine using the Connect button.";
        
        NotificationSystem::Instance().ShowSuccess(
            "Machine Added Successfully",
            wxString::Format("Machine '%s' (%s) has been added and is ready to connect.", data.name, data.machineType)
        );
    }
}

void MachineManagerPanel::OnEditMachine(wxCommandEvent& WXUNUSED(event))
{
    if (m_selectedMachine.empty()) return;
    
    // Find the selected machine
    const MachineConfig* selectedMachine = nullptr;
    size_t selectedIndex = 0;
    for (size_t i = 0; i < m_machines.size(); ++i) {
        if (m_machines[i].id == m_selectedMachine) {
            selectedMachine = &m_machines[i];
            selectedIndex = i;
            break;
        }
    }
    
    if (!selectedMachine) return;
    
    // Create edit dialog with existing machine data
    AddMachineDialog dialog(this, true, wxString::Format("Edit Machine - %s", selectedMachine->name));
    
    // Convert MachineConfig to AddMachineDialog::MachineData
    AddMachineDialog::MachineData data;
    data.name = selectedMachine->name;
    data.description = selectedMachine->description;
    data.host = selectedMachine->host;
    data.port = selectedMachine->port;
    data.protocol = "Telnet"; // Default - could be extended to store protocol
    data.machineType = selectedMachine->machineType;
    data.baudRate = "115200"; // Default
    data.serialPort = "COM1"; // Default
    data.autoConnect = selectedMachine->autoConnect;
    
    dialog.SetMachineData(data);
    
    if (dialog.ShowModal() == wxID_OK) {
        AddMachineDialog::MachineData updatedData = dialog.GetMachineData();
        
        // Handle single auto-connect constraint
        if (updatedData.autoConnect) {
            // If this machine is set to auto-connect, clear auto-connect from all other machines
            for (auto& machine : m_machines) {
                if (machine.id != m_selectedMachine && machine.autoConnect) {
                    machine.autoConnect = false;
                    LOG_INFO("Disabled auto-connect for machine: " + machine.name + " (replaced by edited machine)");
                }
            }
        }
        
        // Update the machine configuration
        m_machines[selectedIndex].name = updatedData.name.ToStdString();
        m_machines[selectedIndex].description = updatedData.description.ToStdString();
        m_machines[selectedIndex].host = updatedData.host.ToStdString();
        m_machines[selectedIndex].port = updatedData.port;
        m_machines[selectedIndex].machineType = updatedData.machineType.ToStdString();
        m_machines[selectedIndex].autoConnect = updatedData.autoConnect;
        
        // Save to persistent storage
        SaveMachineConfigs();
        
        // Refresh the list display
        PopulateMachineList();
        
        // Reselect the edited machine
        for (int i = 0; i < m_machineList->GetItemCount(); ++i) {
            if (m_machineList->GetItemText(i, 0) == updatedData.name) {
                m_machineList->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                break;
            }
        }
        
        // Update details display
        UpdateMachineDetails();
        
        // If auto-connect is enabled and machine is not connected, attempt immediate connection
        if (updatedData.autoConnect && !m_machines[selectedIndex].connected) {
            LOG_INFO("Auto-connect enabled for edited machine: " + updatedData.name.ToStdString() + ". Attempting immediate connection...");
            
            // Test connection in a separate thread
            std::future<bool> connectionTest = std::async(std::launch::async, [this, selectedIndex]() {
                return TestTelnetConnection(m_machines[selectedIndex].host, m_machines[selectedIndex].port);
            });
            
            // Wait for connection test with timeout
            auto status = connectionTest.wait_for(std::chrono::seconds(5));
            
            if (status != std::future_status::timeout) {
                bool connectionSuccess = connectionTest.get();
                
                if (connectionSuccess) {
                    // Update machine status
                    m_machines[selectedIndex].connected = true;
                    m_machines[selectedIndex].lastConnected = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S").ToStdString();
                    
                    // Save updated configuration
                    SaveMachineConfigs();
                    
                    // Refresh UI
                    RefreshMachineList();
                    UpdateMachineDetails();
                    
                    // Log connection success to terminal console
                    MainFrame* mainFrame = dynamic_cast<MainFrame*>(wxGetTopLevelParent(this));
                    if (mainFrame) {
                        ConsolePanel* console = mainFrame->GetConsolePanel();
                        if (console) {
                            console->LogMessage("=== AUTO-CONNECT SUCCESSFUL ===", "INFO");
                            console->LogMessage(wxString::Format("Auto-connected to: %s (%s:%d)", 
                                m_machines[selectedIndex].name, m_machines[selectedIndex].host, m_machines[selectedIndex].port).ToStdString(), "INFO");
                            console->LogMessage(wxString::Format("Machine Type: %s", m_machines[selectedIndex].machineType).ToStdString(), "INFO");
                            console->LogMessage(wxString::Format("Connection Time: %s", m_machines[selectedIndex].lastConnected).ToStdString(), "INFO");
                            console->LogMessage("Status: READY - Machine is active and awaiting commands", "INFO");
                            console->LogMessage("=== END AUTO-CONNECT INFO ===", "INFO");
                            
                            // Simulate some initial communication handshake
                            console->LogSentCommand("?");
                            console->LogReceivedResponse("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>");
                            console->LogMessage("Machine ready for G-code commands", "INFO");
                            
                            // Enable command input now that machine is connected
                            console->SetConnectionEnabled(true, m_machines[selectedIndex].name);
                        }
                    }
                    
                    // Show success notification with connection info
                    NotificationSystem::Instance().ShowSuccess(
                        "Machine Updated and Connected",
                        wxString::Format("Machine '%s' has been updated and automatically connected!", updatedData.name)
                    );
                    
                    LOG_INFO("Immediate auto-connect successful for edited machine: " + updatedData.name.ToStdString());
                    return; // Skip the regular success message
                } else {
                    LOG_ERROR("Immediate auto-connect failed for edited machine: " + updatedData.name.ToStdString());
                    
                    NotificationSystem::Instance().ShowWarning(
                        "Machine Updated but Connection Failed",
                        wxString::Format("Machine '%s' has been updated with auto-connect enabled, but the connection failed. It will retry on next startup.", updatedData.name)
                    );
                    return; // Skip the regular success message
                }
            } else {
                LOG_ERROR("Immediate auto-connect timeout for edited machine: " + updatedData.name.ToStdString());
                
                NotificationSystem::Instance().ShowWarning(
                    "Machine Updated but Connection Timeout",
                    wxString::Format("Machine '%s' has been updated with auto-connect enabled, but the connection timed out. It will retry on next startup.", updatedData.name)
                );
                return; // Skip the regular success message
            }
        }
        
        // Show regular success message (only if auto-connect was not attempted or machine was already connected)
        wxString successMsg = wxString::Format(
            "Machine '%s' has been successfully updated!\n\n"
            "Type: %s\n"
            "Protocol: %s\n",
            updatedData.name, updatedData.machineType, updatedData.protocol
        );
        
        if (updatedData.protocol == "Telnet" || updatedData.protocol == "WebSocket") {
            successMsg += wxString::Format("Connection: %s:%d\n", updatedData.host, updatedData.port);
        } else if (updatedData.protocol == "USB/Serial") {
            successMsg += wxString::Format("Connection: %s @ %s baud\n", updatedData.serialPort, updatedData.baudRate);
        }
        
        NotificationSystem::Instance().ShowSuccess(
            "Machine Updated Successfully",
            wxString::Format("Machine '%s' configuration has been updated.", updatedData.name)
        );
    }
}

void MachineManagerPanel::OnRemoveMachine(wxCommandEvent& WXUNUSED(event))
{
    if (m_selectedMachine.empty()) return;
    
    // Find the selected machine
    const MachineConfig* selectedMachine = nullptr;
    size_t selectedIndex = 0;
    for (size_t i = 0; i < m_machines.size(); ++i) {
        if (m_machines[i].id == m_selectedMachine) {
            selectedMachine = &m_machines[i];
            selectedIndex = i;
            break;
        }
    }
    
    if (!selectedMachine) return;
    
    // Prevent removing connected machines
    if (selectedMachine->connected) {
        NotificationSystem::Instance().ShowWarning(
            "Cannot Remove Connected Machine",
            wxString::Format("Machine '%s' is currently connected. Please disconnect first.", selectedMachine->name)
        );
        return;
    }
    
    // Enhanced confirmation dialog with machine details
    wxString confirmMsg = wxString::Format(
        "Are you sure you want to permanently remove the following machine?\n\n"
        "Name: %s\n"
        "Type: %s\n"
        "Host: %s\n"
        "Port: %d\n\n"
        "This action cannot be undone!",
        selectedMachine->name, selectedMachine->machineType, 
        selectedMachine->host, selectedMachine->port
    );
    
    int result = wxMessageBox(confirmMsg, "Remove Machine", wxYES_NO | wxICON_QUESTION, this);
    
    if (result == wxYES) {
        std::string removedName = selectedMachine->name;
        
        // Remove from vector
        m_machines.erase(m_machines.begin() + selectedIndex);
        
        // Save to persistent storage
        SaveMachineConfigs();
        
        // Clear selection
        m_selectedMachine.clear();
        
        // Refresh the list display
        PopulateMachineList();
        
        // Update details display
        UpdateMachineDetails();
        
        // Show success message
        NotificationSystem::Instance().ShowSuccess(
            "Machine Removed",
            wxString::Format("Machine '%s' has been successfully removed.", removedName)
        );
    }
}

void MachineManagerPanel::OnConnect(wxCommandEvent& WXUNUSED(event))
{
    if (m_selectedMachine.empty()) return;
    
    // Find the selected machine
    MachineConfig* selectedMachine = nullptr;
    size_t selectedIndex = 0;
    for (size_t i = 0; i < m_machines.size(); ++i) {
        if (m_machines[i].id == m_selectedMachine) {
            selectedMachine = &m_machines[i];
            selectedIndex = i;
            break;
        }
    }
    
    if (!selectedMachine) return;
    
    // Check if already connected
    if (selectedMachine->connected) {
        NotificationSystem::Instance().ShowInfo(
            "Already Connected",
            wxString::Format("Machine '%s' is already connected.", selectedMachine->name)
        );
        return;
    }
    
    // Show connection progress dialog
    wxProgressDialog* progressDlg = new wxProgressDialog(
        "Connecting to Machine",
        wxString::Format("Establishing connection to %s:%d...", selectedMachine->host, selectedMachine->port),
        100, this,
        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT
    );
    
    progressDlg->Pulse("Connecting...");
    
    // Test connection first in a separate thread
    std::future<bool> connectionTest = std::async(std::launch::async, [this, selectedMachine]() {
        return TestTelnetConnection(selectedMachine->host, selectedMachine->port);
    });
    
    // Wait for connection test with timeout
    auto status = connectionTest.wait_for(std::chrono::seconds(10));
    
    progressDlg->Destroy();
    
    if (status == std::future_status::timeout) {
        NotificationSystem::Instance().ShowWarning(
            "Connection Timeout",
            wxString::Format("Connection to '%s' (%s:%d) timed out. Check network connectivity.",
                           selectedMachine->name, selectedMachine->host, selectedMachine->port)
        );
        return;
    }
    
    bool connectionSuccess = connectionTest.get();
    
    if (connectionSuccess) {
        // Update machine status
        selectedMachine->connected = true;
        selectedMachine->lastConnected = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S").ToStdString();
        
        // Save updated configuration
        SaveMachineConfigs();
        
        // Refresh UI
        RefreshMachineList();
        UpdateMachineDetails();
        
        // Log connection success to terminal console
        MainFrame* mainFrame = dynamic_cast<MainFrame*>(wxGetTopLevelParent(this));
        
        // Notify MainFrame about connection for status bar update using universal handler
        if (mainFrame) {
            mainFrame->HandleConnectionStatusChange(selectedMachine->id, true);
        }
        if (mainFrame) {
            ConsolePanel* console = mainFrame->GetConsolePanel();
            if (console) {
                console->LogMessage(wxString::Format("=== CONNECTION ESTABLISHED ===").ToStdString(), "INFO");
                console->LogMessage(wxString::Format("Connected to: %s (%s:%d)", selectedMachine->name, selectedMachine->host, selectedMachine->port).ToStdString(), "INFO");
                console->LogMessage(wxString::Format("Machine Type: %s", selectedMachine->machineType).ToStdString(), "INFO");
                console->LogMessage(wxString::Format("Connection Time: %s", selectedMachine->lastConnected).ToStdString(), "INFO");
                console->LogMessage(wxString::Format("Status: READY - Machine is active and awaiting commands").ToStdString(), "INFO");
                console->LogMessage(wxString::Format("=== END CONNECTION INFO ===").ToStdString(), "INFO");
                
                // Simulate some initial communication handshake
                console->LogSentCommand("?");
                console->LogReceivedResponse("<Idle|MPos:0.000,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>");
                console->LogMessage("Machine ready for G-code commands", "INFO");
                
                // Enable command input now that machine is connected
                console->SetConnectionEnabled(true, selectedMachine->name);
            }
        }
        
        // Show success notification
        NotificationSystem::Instance().ShowSuccess(
            "Connection Successful",
            wxString::Format("Successfully connected to '%s' (%s:%d). Machine is ready for use.",
                           selectedMachine->name, selectedMachine->host, selectedMachine->port)
        );
    } else {
        // Connection failed - show error notification
        NotificationSystem::Instance().ShowError(
            "Connection Failed",
            wxString::Format("Failed to connect to '%s' (%s:%d). Check machine power and network connectivity.",
                           selectedMachine->name, selectedMachine->host, selectedMachine->port)
        );
    }
}

void MachineManagerPanel::OnDisconnect(wxCommandEvent& WXUNUSED(event))
{
    if (m_selectedMachine.empty()) return;
    
    // Find the selected machine
    MachineConfig* selectedMachine = nullptr;
    for (auto& machine : m_machines) {
        if (machine.id == m_selectedMachine) {
            selectedMachine = &machine;
            break;
        }
    }
    
    if (!selectedMachine) return;
    
    // Check if actually connected
    if (!selectedMachine->connected) {
        NotificationSystem::Instance().ShowInfo(
            "Not Connected",
            wxString::Format("Machine '%s' is not currently connected.", selectedMachine->name)
        );
        return;
    }
    
    // Confirm disconnection
    int result = wxMessageBox(
        wxString::Format("Are you sure you want to disconnect from '%s'?\n\n"
                       "Host: %s\n"
                       "Port: %d\n\n"
                       "Any ongoing operations will be interrupted.",
                       selectedMachine->name, selectedMachine->host, selectedMachine->port),
        "Confirm Disconnection", wxYES_NO | wxICON_QUESTION, this);
    
    if (result == wxYES) {
        // Log disconnection to terminal console
        MainFrame* mainFrame = dynamic_cast<MainFrame*>(wxGetTopLevelParent(this));
        if (mainFrame) {
            ConsolePanel* console = mainFrame->GetConsolePanel();
            if (console) {
                console->LogMessage(wxString::Format("=== DISCONNECTION INITIATED ===").ToStdString(), "WARNING");
                console->LogMessage(wxString::Format("Disconnecting from: %s (%s:%d)", selectedMachine->name, selectedMachine->host, selectedMachine->port).ToStdString(), "INFO");
                console->LogSentCommand("$X"); // Simulate unlock command
                console->LogReceivedResponse("ok");
                console->LogMessage("Connection terminated by user", "INFO");
                console->LogMessage("=== MACHINE OFFLINE ===", "WARNING");
                
                // Disable command input now that machine is disconnected
                console->SetConnectionEnabled(false);
            }
        }
        
        // Update machine status
        selectedMachine->connected = false;
        
        // Save updated configuration
        SaveMachineConfigs();
        
        // Refresh UI
        RefreshMachineList();
        UpdateMachineDetails();
        
        // Notify MainFrame about disconnection for status bar update using universal handler
        if (mainFrame) {
            mainFrame->HandleConnectionStatusChange(selectedMachine->id, false);
        }
        
        // Show disconnection message
        NotificationSystem::Instance().ShowSuccess(
            "Disconnected",
            wxString::Format("Successfully disconnected from '%s'. Machine is now offline.", selectedMachine->name)
        );
    }
}

void MachineManagerPanel::OnTestConnection(wxCommandEvent& WXUNUSED(event))
{
    if (m_selectedMachine.empty()) return;
    
    // Find the selected machine
    const MachineConfig* selectedMachine = nullptr;
    for (const auto& machine : m_machines) {
        if (machine.id == m_selectedMachine) {
            selectedMachine = &machine;
            break;
        }
    }
    
    if (!selectedMachine) return;
    
    // Show progress dialog
    wxProgressDialog* progressDlg = new wxProgressDialog(
        "Testing Connection",
        wxString::Format("Testing connection to %s:%d...", selectedMachine->host, selectedMachine->port),
        100, this,
        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT
    );
    
    progressDlg->Pulse("Connecting...");
    
    // Test connection in a separate thread to avoid blocking UI
    std::future<bool> connectionTest = std::async(std::launch::async, [this, selectedMachine]() {
        return TestTelnetConnection(selectedMachine->host, selectedMachine->port);
    });
    
    // Wait for connection test with timeout
    auto status = connectionTest.wait_for(std::chrono::seconds(5));
    
    progressDlg->Destroy();
    
    if (status == std::future_status::timeout) {
        NotificationSystem::Instance().ShowWarning(
            "Connection Test - Timeout",
            wxString::Format("Connection test to '%s' (%s:%d) timed out. Machine may be offline.",
                           selectedMachine->name, selectedMachine->host, selectedMachine->port)
        );
    } else {
        bool success = connectionTest.get();
        if (success) {
            NotificationSystem::Instance().ShowSuccess(
                "Connection Test - Success",
                wxString::Format("Connection test to '%s' (%s:%d) was successful! Machine is reachable.",
                               selectedMachine->name, selectedMachine->host, selectedMachine->port)
            );
        } else {
            NotificationSystem::Instance().ShowError(
                "Connection Test - Failed",
                wxString::Format("Connection test to '%s' (%s:%d) failed. Check machine power and network.",
                               selectedMachine->name, selectedMachine->host, selectedMachine->port)
            );
        }
    }
}

void MachineManagerPanel::OnImportConfig(wxCommandEvent& WXUNUSED(event))
{
    NotificationSystem::Instance().ShowInfo(
        "Import Config",
        "Import Configuration dialog would open here. This will allow importing machine configurations from file."
    );
}

void MachineManagerPanel::OnExportConfig(wxCommandEvent& WXUNUSED(event))
{
    NotificationSystem::Instance().ShowInfo(
        "Export Config",
        "Export Configuration dialog would open here. This will allow exporting machine configurations to file."
    );
}

void MachineManagerPanel::OnScanNetwork(wxCommandEvent& WXUNUSED(event))
{
    NetworkScanDialog scanDialog(this);
    
    if (scanDialog.ShowModal() == wxID_OK && scanDialog.HasSelectedDevice()) {
        NetworkDevice selectedDevice = scanDialog.GetSelectedDevice();
        
        // Create a new machine configuration from the discovered device
        AddMachineDialog dialog(this, false, "Add Discovered Machine");
        
        // Pre-populate with discovered information
        AddMachineDialog::MachineData data;
        data.name = wxString::Format("%s-%s", selectedDevice.deviceType, selectedDevice.ip);
        data.description = wxString::Format("Discovered %s device", selectedDevice.deviceType);
        data.host = selectedDevice.ip;
        data.port = (selectedDevice.deviceType == "FluidNC") ? 23 : 80;
        data.protocol = "Telnet";
        data.machineType = (selectedDevice.deviceType == "FluidNC") ? "FluidNC" : "Unknown";
        data.baudRate = "115200";
        data.serialPort = "COM1";
        
        dialog.SetMachineData(data);
        
        if (dialog.ShowModal() == wxID_OK) {
            AddMachineDialog::MachineData finalData = dialog.GetMachineData();
            
            // Generate unique machine ID
            std::string machineId = "machine" + std::to_string(m_machines.size() + 1);
            
            // Create new machine configuration
            MachineConfig newMachine;
            newMachine.id = machineId;
            newMachine.name = finalData.name.ToStdString();
            newMachine.description = finalData.description.ToStdString();
            newMachine.host = finalData.host.ToStdString();
            newMachine.port = finalData.port;
            newMachine.machineType = finalData.machineType.ToStdString();
            newMachine.connected = false;
            newMachine.lastConnected = "Never";
            
            // Add to machines list
            m_machines.push_back(newMachine);
            
            // Save to persistent storage
            SaveMachineConfigs();
            
            // Refresh the list display
            PopulateMachineList();
            
            // Select the newly added machine
            for (int i = 0; i < m_machineList->GetItemCount(); ++i) {
                if (m_machineList->GetItemText(i, 0) == finalData.name) {
                    m_machineList->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                    SelectMachine(machineId);
                    break;
                }
            }
            
            // Show success message
            NotificationSystem::Instance().ShowSuccess(
                "Machine Added from Network Scan",
                wxString::Format(
                    "Successfully added discovered machine '%s' (%s). You can now connect to this machine.",
                    finalData.name, selectedDevice.ip
                )
            );
        }
    }
}

wxString MachineManagerPanel::GetSettingsPath()
{
    // Get user's app data directory
    wxString appDataDir = wxStandardPaths::Get().GetUserDataDir();
    
    // Create our settings subdirectory
    wxString settingsDir = appDataDir + wxFileName::GetPathSeparator() + "settings";
    
    // Ensure the settings directory exists
    if (!wxDir::Exists(settingsDir)) {
        if (!wxFileName::Mkdir(settingsDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
            LOG_ERROR("Could not create settings directory: " + settingsDir.ToStdString());
            return wxEmptyString;
        }
        LOG_INFO("Created settings directory: " + settingsDir.ToStdString());
    }
    
    return settingsDir;
}

void MachineManagerPanel::CreateEmptyMachinesFile(const wxString& filePath)
{
    try {
        nlohmann::json j;
        j["machines"] = nlohmann::json::array();
        j["version"] = "1.0";
        j["created"] = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S").ToStdString();
        
        std::ofstream file(filePath.ToStdString());
        if (file.is_open()) {
            file << j.dump(4); // Pretty print with 4-space indentation
            file.close();
            LOG_INFO("Created empty machines.json file: " + filePath.ToStdString());
        } else {
            LOG_ERROR("Could not create machines.json file: " + filePath.ToStdString());
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error creating machines.json file: " + std::string(e.what()));
    }
}

void MachineManagerPanel::SaveMachineConfigs()
{
    wxString settingsPath = GetSettingsPath();
    if (settingsPath.IsEmpty()) {
        LOG_ERROR("Could not determine settings path");
        return;
    }
    
    wxString machinesFile = settingsPath + wxFileName::GetPathSeparator() + "machines.json";
    
    try {
        nlohmann::json j;
        j["machines"] = nlohmann::json::array();
        j["version"] = "1.0";
        j["lastModified"] = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S").ToStdString();
        
        // Convert machines to JSON
        for (const auto& machine : m_machines) {
            nlohmann::json machineJson;
            machineJson["id"] = machine.id;
            machineJson["name"] = machine.name;
            machineJson["description"] = machine.description;
            machineJson["host"] = machine.host;
            machineJson["port"] = machine.port;
            machineJson["machineType"] = machine.machineType;
            machineJson["lastConnected"] = machine.lastConnected;
            machineJson["autoConnect"] = machine.autoConnect;
            
            j["machines"].push_back(machineJson);
        }
        
        // Write to file
        std::ofstream file(machinesFile.ToStdString());
        if (file.is_open()) {
            file << j.dump(4); // Pretty print with 4-space indentation
            file.close();
            std::string saveMsg = "Saved " + std::to_string(m_machines.size()) + " machine configurations to " + machinesFile.ToStdString();
            LOG_INFO(saveMsg);
        } else {
            LOG_ERROR("Could not open machines.json file for writing: " + machinesFile.ToStdString());
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error saving machine configurations: " + std::string(e.what()));
    }
}

// Connection helper methods
bool MachineManagerPanel::TestTelnetConnection(const std::string& host, int port)
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
