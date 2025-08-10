/**
 * gui/MachineManagerPanel.cpp
 * Machine Manager Panel implementation with dummy content
 */

#include "MachineManagerPanel.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>

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
    
    m_splitter->SplitVertically(m_listPanel, m_detailsPanel, 300);
    
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
    
    // Details grid
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(6, 2, 5, 10);
    gridSizer->AddGrowableCol(1, 1);
    
    gridSizer->Add(new wxStaticText(m_detailsPanel, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
    m_nameLabel = new wxStaticText(m_detailsPanel, wxID_ANY, "-");
    gridSizer->Add(m_nameLabel, 1, wxEXPAND);
    
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
    // Create some dummy machine configurations
    m_machines.clear();
    
    MachineConfig machine1;
    machine1.id = "machine1";
    machine1.name = "CNC Router";
    machine1.host = "192.168.1.100";
    machine1.port = 23;
    machine1.machineType = "FluidNC";
    machine1.connected = false;
    machine1.lastConnected = "Never";
    m_machines.push_back(machine1);
    
    MachineConfig machine2;
    machine2.id = "machine2";
    machine2.name = "Laser Engraver";
    machine2.host = "192.168.1.101";
    machine2.port = 23;
    machine2.machineType = "FluidNC";
    machine2.connected = false;
    machine2.lastConnected = "2025-01-09 14:30:00";
    m_machines.push_back(machine2);
    
    MachineConfig machine3;
    machine3.id = "machine3";
    machine3.name = "3D Printer";
    machine3.host = "localhost";
    machine3.port = 8080;
    machine3.machineType = "Marlin";
    machine3.connected = true;
    machine3.lastConnected = "Connected";
    m_machines.push_back(machine3);
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
    wxMessageBox("Add Machine dialog would open here.\n\nThis will allow creating new machine configurations.",
                 "Add Machine", wxOK | wxICON_INFORMATION, this);
}

void MachineManagerPanel::OnEditMachine(wxCommandEvent& WXUNUSED(event))
{
    if (m_selectedMachine.empty()) return;
    
    wxMessageBox(wxString::Format("Edit Machine dialog would open here for machine: %s\n\nThis will allow editing machine configuration.",
                                 m_selectedMachine), "Edit Machine", wxOK | wxICON_INFORMATION, this);
}

void MachineManagerPanel::OnRemoveMachine(wxCommandEvent& WXUNUSED(event))
{
    if (m_selectedMachine.empty()) return;
    
    int result = wxMessageBox(wxString::Format("Are you sure you want to remove machine: %s?", m_selectedMachine),
                             "Remove Machine", wxYES_NO | wxICON_QUESTION, this);
    
    if (result == wxYES) {
        // Remove from vector (simplified)
        wxMessageBox("Machine would be removed here.", "Remove Machine", wxOK | wxICON_INFORMATION, this);
    }
}

void MachineManagerPanel::OnConnect(wxCommandEvent& WXUNUSED(event))
{
    if (m_selectedMachine.empty()) return;
    
    wxMessageBox(wxString::Format("Connecting to machine: %s\n\nThis will establish a connection to the selected machine.",
                                 m_selectedMachine), "Connect", wxOK | wxICON_INFORMATION, this);
}

void MachineManagerPanel::OnDisconnect(wxCommandEvent& WXUNUSED(event))
{
    if (m_selectedMachine.empty()) return;
    
    wxMessageBox(wxString::Format("Disconnecting from machine: %s", m_selectedMachine),
                 "Disconnect", wxOK | wxICON_INFORMATION, this);
}

void MachineManagerPanel::OnTestConnection(wxCommandEvent& WXUNUSED(event))
{
    if (m_selectedMachine.empty()) return;
    
    wxMessageBox(wxString::Format("Testing connection to machine: %s\n\nThis will test the connection without establishing a permanent connection.",
                                 m_selectedMachine), "Test Connection", wxOK | wxICON_INFORMATION, this);
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

void MachineManagerPanel::SaveMachineConfigs()
{
    // TODO: Implement configuration persistence
}
