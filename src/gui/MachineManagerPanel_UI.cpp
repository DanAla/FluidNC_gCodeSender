/**
 * gui/MachineManagerPanel_UI.cpp
 *
 * UI creation implementation for the MachineManagerPanel class.
 */

#include "MachineManagerPanel.h"
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

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
    
    m_importBtn = new wxButton(m_listPanel, ID_MM_IMPORT_CONFIG, "Import");
    m_exportBtn = new wxButton(m_listPanel, ID_MM_EXPORT_CONFIG, "Export");
    
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
    m_testBtn = new wxButton(m_detailsPanel, ID_MM_TEST_CONNECTION, "Test");
    
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
