/**
 * gui/MachineManagerPanel.cpp
 * Machine Manager Panel implementation with dummy content
 */

#include "MachineManagerPanel.h"
#include "AddMachineDialog.h"
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
    m_descriptionLabel = new wxStaticText(m_detailsPanel, wxID_ANY, "-");
    m_descriptionLabel->Wrap(300); // Allow text wrapping for long descriptions
    gridSizer->Add(m_descriptionLabel, 1, wxEXPAND);
    
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
            wxLogWarning("Could not open machines.json file for reading");
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
                
                m_machines.push_back(machine);
            }
        }
        
        wxLogMessage("Loaded %zu machine configurations from %s", m_machines.size(), machinesFile);
        
    } catch (const std::exception& e) {
        wxLogError("Error loading machine configurations: %s", e.what());
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
        
        // Color coding
        if (machine.connected) {
            m_machineList->SetItemTextColour(itemIndex, *wxGREEN);
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
        m_descriptionLabel->Wrap(300); // Re-wrap after setting new text
        
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
        
        // Color status label
        if (selectedMachine->connected) {
            m_statusLabel->SetForegroundColour(*wxGREEN);
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
        
        // Show success message
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
        
        wxMessageBox(successMsg, "Machine Added Successfully", wxOK | wxICON_INFORMATION, this);
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
    
    dialog.SetMachineData(data);
    
    if (dialog.ShowModal() == wxID_OK) {
        AddMachineDialog::MachineData updatedData = dialog.GetMachineData();
        
        // Update the machine configuration
        m_machines[selectedIndex].name = updatedData.name.ToStdString();
        m_machines[selectedIndex].description = updatedData.description.ToStdString();
        m_machines[selectedIndex].host = updatedData.host.ToStdString();
        m_machines[selectedIndex].port = updatedData.port;
        m_machines[selectedIndex].machineType = updatedData.machineType.ToStdString();
        
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
        
        // Show success message
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
        
        wxMessageBox(successMsg, "Machine Updated Successfully", wxOK | wxICON_INFORMATION, this);
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
        wxMessageBox(
            wxString::Format("Cannot remove machine '%s' because it is currently connected.\n\n"
                           "Please disconnect the machine first before removing it.",
                           selectedMachine->name),
            "Cannot Remove Connected Machine", wxOK | wxICON_WARNING, this);
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
        wxMessageBox(
            wxString::Format("Machine '%s' has been successfully removed.", removedName),
            "Machine Removed", wxOK | wxICON_INFORMATION, this);
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
        wxMessageBox(
            wxString::Format("Machine '%s' is already connected.\n\n"
                           "Use the Disconnect button to disconnect first.",
                           selectedMachine->name),
            "Already Connected", wxOK | wxICON_INFORMATION, this);
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
        wxMessageBox(
            wxString::Format("Connection to '%s' timed out.\n\n"
                           "Host: %s\n"
                           "Port: %d\n\n"
                           "Please check your connection and try again.",
                           selectedMachine->name, selectedMachine->host, selectedMachine->port),
            "Connection Timeout", wxOK | wxICON_WARNING, this);
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
        
        // Show success message
        wxMessageBox(
            wxString::Format("Successfully connected to '%s'!\n\n"
                           "Host: %s\n"
                           "Port: %d\n"
                           "Type: %s\n\n"
                           "The machine is now active and ready for use.",
                           selectedMachine->name, selectedMachine->host, 
                           selectedMachine->port, selectedMachine->machineType),
            "Connection Successful", wxOK | wxICON_INFORMATION, this);
    } else {
        // Connection failed
        wxMessageBox(
            wxString::Format("Failed to connect to '%s'.\n\n"
                           "Host: %s\n"
                           "Port: %d\n\n"
                           "Possible causes:\n"
                           "- Machine is powered off or not responding\n"
                           "- Network connection issues\n"
                           "- Incorrect host address or port\n"
                           "- Firewall blocking the connection\n"
                           "- Machine is busy or in an error state\n\n"
                           "Please check the machine and try again.",
                           selectedMachine->name, selectedMachine->host, selectedMachine->port),
            "Connection Failed", wxOK | wxICON_ERROR, this);
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
        wxMessageBox(
            wxString::Format("Machine '%s' is not currently connected.",
                           selectedMachine->name),
            "Not Connected", wxOK | wxICON_INFORMATION, this);
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
        // Update machine status
        selectedMachine->connected = false;
        
        // Save updated configuration
        SaveMachineConfigs();
        
        // Refresh UI
        RefreshMachineList();
        UpdateMachineDetails();
        
        // Show disconnection message
        wxMessageBox(
            wxString::Format("Successfully disconnected from '%s'.\n\n"
                           "The machine is now offline and no longer active.",
                           selectedMachine->name),
            "Disconnected", wxOK | wxICON_INFORMATION, this);
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
        wxMessageBox(
            wxString::Format("Connection test to '%s' timed out.\n\n"
                           "Host: %s\n"
                           "Port: %d\n\n"
                           "The machine may be offline or unreachable.",
                           selectedMachine->name, selectedMachine->host, selectedMachine->port),
            "Connection Test - Timeout", wxOK | wxICON_WARNING, this);
    } else {
        bool success = connectionTest.get();
        if (success) {
            wxMessageBox(
                wxString::Format("Connection test to '%s' was successful!\n\n"
                               "Host: %s\n"
                               "Port: %d\n\n"
                               "The machine is reachable and accepting connections.",
                               selectedMachine->name, selectedMachine->host, selectedMachine->port),
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
                               selectedMachine->name, selectedMachine->host, selectedMachine->port),
                "Connection Test - Failed", wxOK | wxICON_ERROR, this);
        }
    }
}

void MachineManagerPanel::OnImportConfig(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox("Import Configuration dialog would open here.\n\nThis will allow importing machine configurations from file.",
                 "Import Config", wxOK | wxICON_INFORMATION, this);
}

void MachineManagerPanel::OnExportConfig(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox("Export Configuration dialog would open here.\n\nThis will allow exporting machine configurations to file.",
                 "Export Config", wxOK | wxICON_INFORMATION, this);
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
            wxLogError("Could not create settings directory: %s", settingsDir);
            return wxEmptyString;
        }
        wxLogMessage("Created settings directory: %s", settingsDir);
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
            wxLogMessage("Created empty machines.json file: %s", filePath);
        } else {
            wxLogError("Could not create machines.json file: %s", filePath);
        }
    } catch (const std::exception& e) {
        wxLogError("Error creating machines.json file: %s", e.what());
    }
}

void MachineManagerPanel::SaveMachineConfigs()
{
    wxString settingsPath = GetSettingsPath();
    if (settingsPath.IsEmpty()) {
        wxLogError("Could not determine settings path");
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
            
            j["machines"].push_back(machineJson);
        }
        
        // Write to file
        std::ofstream file(machinesFile.ToStdString());
        if (file.is_open()) {
            file << j.dump(4); // Pretty print with 4-space indentation
            file.close();
            wxLogMessage("Saved %zu machine configurations to %s", m_machines.size(), machinesFile);
        } else {
            wxLogError("Could not open machines.json file for writing: %s", machinesFile);
        }
        
    } catch (const std::exception& e) {
        wxLogError("Error saving machine configurations: %s", e.what());
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
