/**
 * gui/EnhancedEditMachineDialog.cpp
 * Implementation of enhanced machine configuration dialog
 */

#include "EnhancedEditMachineDialog.h"
#include "../core/CommunicationManager.h"
#include "../core/SimpleLogger.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/datetime.h>

wxBEGIN_EVENT_TABLE(EnhancedEditMachineDialog, wxDialog)
    EVT_BUTTON(wxID_OK, EnhancedEditMachineDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, EnhancedEditMachineDialog::OnCancel)
    EVT_BUTTON(ID_APPLY_SETTINGS, EnhancedEditMachineDialog::OnApply)
    EVT_BUTTON(ID_QUERY_MACHINE, EnhancedEditMachineDialog::OnQueryMachine)
    EVT_BUTTON(ID_TEST_HOMING, EnhancedEditMachineDialog::OnTestHoming)
    EVT_BUTTON(ID_AUTO_DETECT_KINEMATICS, EnhancedEditMachineDialog::OnAutoDetectKinematics)
    EVT_CHOICE(ID_HOMING_SEQUENCE_CHOICE, EnhancedEditMachineDialog::OnHomingSequenceChanged)
    EVT_BUTTON(ID_ADD_HOMING_STEP, EnhancedEditMachineDialog::OnAddHomingStep)
    EVT_BUTTON(ID_REMOVE_HOMING_STEP, EnhancedEditMachineDialog::OnRemoveHomingStep)
    EVT_BUTTON(ID_MOVE_STEP_UP, EnhancedEditMachineDialog::OnMoveHomingStepUp)
    EVT_BUTTON(ID_MOVE_STEP_DOWN, EnhancedEditMachineDialog::OnMoveHomingStepDown)
    EVT_LIST_ITEM_SELECTED(ID_CUSTOM_SEQUENCE_LIST, EnhancedEditMachineDialog::OnHomingStepSelected)
    EVT_LIST_ITEM_ACTIVATED(ID_CUSTOM_SEQUENCE_LIST, EnhancedEditMachineDialog::OnHomingStepEdit)
wxEND_EVENT_TABLE()

EnhancedEditMachineDialog::EnhancedEditMachineDialog(wxWindow* parent, const std::string& machineId, bool isNewMachine)
    : wxDialog(parent, wxID_ANY, isNewMachine ? "Add New Machine" : "Edit Machine", wxDefaultPosition, wxSize(600, 500), 
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_machineId(machineId),
      m_isNewMachine(isNewMachine),
      m_notebook(nullptr),
      m_progressDialog(nullptr),
      m_queryInProgress(false),
      m_homingInProgress(false)
{
    // Initialize predefined choices
    m_machineTypes = {"FluidNC", "GRBL", "Custom"};
    m_kinematicsTypes = {"Cartesian", "CoreXY", "Delta", "SCARA", "Custom"};
    m_coordinateSystems = {"G54", "G55", "G56", "G57", "G58", "G59"};
    
    // Load existing machine data if editing
    if (!isNewMachine && !machineId.empty()) {
        m_config = MachineConfigManager::Instance().GetMachine(machineId);
        if (m_config.id.empty()) {
            // Machine not found, treat as new
            m_isNewMachine = true;
            m_config = EnhancedMachineConfig{};
            m_config.id = machineId;
        }
    } else {
        // Initialize new machine with defaults
        m_config = EnhancedMachineConfig{};
        if (!machineId.empty()) {
            m_config.id = machineId;
        }
    }
    
    CreateNotebook();
    
    // Create main sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(m_notebook, 1, wxALL | wxEXPAND, 10);
    
    // Create button sizer
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_okBtn = new wxButton(this, wxID_OK, "OK");
    m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    m_applyBtn = new wxButton(this, ID_APPLY_SETTINGS, "Apply");
    
    buttonSizer->Add(m_okBtn, 0, wxRIGHT, 5);
    buttonSizer->Add(m_cancelBtn, 0, wxRIGHT, 5);
    buttonSizer->Add(m_applyBtn, 0, 0);
    
    mainSizer->Add(buttonSizer, 0, wxALL | wxALIGN_RIGHT, 10);
    
    SetSizer(mainSizer);
    
    // Load current machine data into controls
    LoadMachineData();
    
    // Update UI state
    UpdateUI();
    
    // Set focus to name field for new machines
    if (m_isNewMachine && m_nameText) {
        m_nameText->SetFocus();
    }
}

EnhancedEditMachineDialog::~EnhancedEditMachineDialog() {
    if (m_progressDialog) {
        m_progressDialog->Destroy();
        m_progressDialog = nullptr;
    }
}

void EnhancedEditMachineDialog::CreateNotebook() {
    m_notebook = new wxNotebook(this, wxID_ANY);
    
    CreateBasicTab(m_notebook);
    CreateCapabilitiesTab(m_notebook);
    CreateHomingTab(m_notebook);
    CreateUserSettingsTab(m_notebook);
}

void EnhancedEditMachineDialog::CreateBasicTab(wxNotebook* notebook) {
    m_basicPanel = new wxPanel(notebook);
    notebook->AddPage(m_basicPanel, "Basic Settings", true);
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Machine identification section
    wxStaticBoxSizer* identificationBox = new wxStaticBoxSizer(wxVERTICAL, m_basicPanel, "Machine Identification");
    
    wxFlexGridSizer* identificationGrid = new wxFlexGridSizer(3, 2, 5, 10);
    identificationGrid->AddGrowableCol(1, 1);
    
    // Name
    identificationGrid->Add(new wxStaticText(m_basicPanel, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
    m_nameText = new wxTextCtrl(m_basicPanel, wxID_ANY, "");
    m_nameText->SetToolTip("Unique name for this machine configuration");
    identificationGrid->Add(m_nameText, 1, wxEXPAND);
    
    // Description
    identificationGrid->Add(new wxStaticText(m_basicPanel, wxID_ANY, "Description:"), 0, wxALIGN_TOP);
    m_descriptionText = new wxTextCtrl(m_basicPanel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 60), wxTE_MULTILINE);
    m_descriptionText->SetToolTip("Optional description or notes about this machine");
    identificationGrid->Add(m_descriptionText, 1, wxEXPAND);
    
    // Machine Type
    identificationGrid->Add(new wxStaticText(m_basicPanel, wxID_ANY, "Type:"), 0, wxALIGN_CENTER_VERTICAL);
    m_machineTypeChoice = new wxChoice(m_basicPanel, wxID_ANY);
    for (const auto& type : m_machineTypes) {
        m_machineTypeChoice->Append(type);
    }
    m_machineTypeChoice->SetSelection(0); // Default to FluidNC
    m_machineTypeChoice->SetToolTip("Machine firmware type");
    identificationGrid->Add(m_machineTypeChoice, 1, wxEXPAND);
    
    identificationBox->Add(identificationGrid, 1, wxALL | wxEXPAND, 5);
    mainSizer->Add(identificationBox, 0, wxALL | wxEXPAND, 5);
    
    // Connection settings section
    CreateConnectionSettings(m_basicPanel);
    wxStaticBoxSizer* connectionBox = new wxStaticBoxSizer(wxVERTICAL, m_basicPanel, "Connection Settings");
    
    wxFlexGridSizer* connectionGrid = new wxFlexGridSizer(2, 2, 5, 10);
    connectionGrid->AddGrowableCol(1, 1);
    
    // Host/IP Address
    connectionGrid->Add(new wxStaticText(m_basicPanel, wxID_ANY, "Host:"), 0, wxALIGN_CENTER_VERTICAL);
    m_hostText = new wxTextCtrl(m_basicPanel, wxID_ANY, "");
    m_hostText->SetToolTip("IP address or hostname of the FluidNC machine");
    connectionGrid->Add(m_hostText, 1, wxEXPAND);
    
    // Port
    connectionGrid->Add(new wxStaticText(m_basicPanel, wxID_ANY, "Port:"), 0, wxALIGN_CENTER_VERTICAL);
    m_portSpinner = new wxSpinCtrl(m_basicPanel, wxID_ANY);
    m_portSpinner->SetRange(1, 65535);
    m_portSpinner->SetValue(23); // Default telnet port
    m_portSpinner->SetToolTip("TCP port number (usually 23 for FluidNC telnet)");
    connectionGrid->Add(m_portSpinner, 1, wxEXPAND);
    
    connectionBox->Add(connectionGrid, 1, wxALL | wxEXPAND, 5);
    
    // Auto-connect option
    m_autoConnectCheck = new wxCheckBox(m_basicPanel, wxID_ANY, "Auto-connect on startup");
    m_autoConnectCheck->SetToolTip("Automatically connect to this machine when the application starts");
    connectionBox->Add(m_autoConnectCheck, 0, wxALL, 5);
    
    mainSizer->Add(connectionBox, 0, wxALL | wxEXPAND, 5);
    
    // Add spacer
    mainSizer->AddStretchSpacer(1);
    
    m_basicPanel->SetSizer(mainSizer);
}

void EnhancedEditMachineDialog::CreateConnectionSettings(wxPanel* panel) {
    // This method is called from CreateBasicTab - connection settings are created there
    // This method can be used for additional connection-specific logic if needed
}

void EnhancedEditMachineDialog::CreateCapabilitiesTab(wxNotebook* notebook) {
    m_capabilitiesPanel = new wxPanel(notebook);
    notebook->AddPage(m_capabilitiesPanel, "Machine Capabilities");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Auto-discovery section
    wxStaticBoxSizer* discoveryBox = new wxStaticBoxSizer(wxVERTICAL, m_capabilitiesPanel, "Machine Discovery");
    
    wxBoxSizer* discoveryBtnSizer = new wxBoxSizer(wxHORIZONTAL);
    m_queryMachineBtn = new wxButton(m_capabilitiesPanel, ID_QUERY_MACHINE, "Query Machine");
    m_queryMachineBtn->SetToolTip("Connect to machine and discover all capabilities automatically");
    m_autoDetectBtn = new wxButton(m_capabilitiesPanel, ID_AUTO_DETECT_KINEMATICS, "Auto-Detect Kinematics");
    m_autoDetectBtn->SetToolTip("Automatically detect machine kinematics type from settings");
    
    discoveryBtnSizer->Add(m_queryMachineBtn, 0, wxRIGHT, 5);
    discoveryBtnSizer->Add(m_autoDetectBtn, 0, 0);
    
    discoveryBox->Add(discoveryBtnSizer, 0, wxALL, 5);
    mainSizer->Add(discoveryBox, 0, wxALL | wxEXPAND, 5);
    
    // Discovered capabilities section
    wxStaticBoxSizer* capabilitiesBox = new wxStaticBoxSizer(wxVERTICAL, m_capabilitiesPanel, "Discovered Capabilities");
    
    wxFlexGridSizer* capabilitiesGrid = new wxFlexGridSizer(5, 2, 5, 10);
    capabilitiesGrid->AddGrowableCol(1, 1);
    
    // Firmware Version
    capabilitiesGrid->Add(new wxStaticText(m_capabilitiesPanel, wxID_ANY, "Firmware:"), 0, wxALIGN_CENTER_VERTICAL);
    m_firmwareVersionLabel = new wxStaticText(m_capabilitiesPanel, wxID_ANY, "Not queried");
    capabilitiesGrid->Add(m_firmwareVersionLabel, 1, wxEXPAND);
    
    // Kinematics
    capabilitiesGrid->Add(new wxStaticText(m_capabilitiesPanel, wxID_ANY, "Kinematics:"), 0, wxALIGN_CENTER_VERTICAL);
    m_kinematicsLabel = new wxStaticText(m_capabilitiesPanel, wxID_ANY, "Not queried");
    capabilitiesGrid->Add(m_kinematicsLabel, 1, wxEXPAND);
    
    // Workspace Bounds
    capabilitiesGrid->Add(new wxStaticText(m_capabilitiesPanel, wxID_ANY, "Workspace:"), 0, wxALIGN_CENTER_VERTICAL);
    m_workspaceBoundsLabel = new wxStaticText(m_capabilitiesPanel, wxID_ANY, "Not queried");
    capabilitiesGrid->Add(m_workspaceBoundsLabel, 1, wxEXPAND);
    
    // Features
    capabilitiesGrid->Add(new wxStaticText(m_capabilitiesPanel, wxID_ANY, "Features:"), 0, wxALIGN_CENTER_VERTICAL);
    m_featuresLabel = new wxStaticText(m_capabilitiesPanel, wxID_ANY, "Not queried");
    capabilitiesGrid->Add(m_featuresLabel, 1, wxEXPAND);
    
    // Last Queried
    capabilitiesGrid->Add(new wxStaticText(m_capabilitiesPanel, wxID_ANY, "Last Queried:"), 0, wxALIGN_CENTER_VERTICAL);
    m_lastQueriedLabel = new wxStaticText(m_capabilitiesPanel, wxID_ANY, "Never");
    capabilitiesGrid->Add(m_lastQueriedLabel, 1, wxEXPAND);
    
    capabilitiesBox->Add(capabilitiesGrid, 0, wxALL | wxEXPAND, 5);
    
    // Detailed capability information
    capabilitiesBox->Add(new wxStaticText(m_capabilitiesPanel, wxID_ANY, "Detailed Information:"), 0, wxALL, 5);
    m_capabilityDetails = new wxTextCtrl(m_capabilitiesPanel, wxID_ANY, "Use 'Query Machine' to discover capabilities...", 
                                         wxDefaultPosition, wxSize(-1, 150), wxTE_MULTILINE | wxTE_READONLY);
    capabilitiesBox->Add(m_capabilityDetails, 1, wxALL | wxEXPAND, 5);
    
    mainSizer->Add(capabilitiesBox, 1, wxALL | wxEXPAND, 5);
    
    m_capabilitiesPanel->SetSizer(mainSizer);
}

// Get the configured machine data
EnhancedMachineConfig EnhancedEditMachineDialog::GetMachineConfig() const {
    return m_config;
}

// Set machine data for editing
void EnhancedEditMachineDialog::SetMachineConfig(const EnhancedMachineConfig& config) {
    m_config = config;
    m_machineId = config.id;
    LoadMachineData();
    UpdateUI();
}

// Event handlers
void EnhancedEditMachineDialog::OnOK(wxCommandEvent& event) {
    if (ValidateInput()) {
        SaveMachineData();
        EndModal(wxID_OK);
    }
}

void EnhancedEditMachineDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void EnhancedEditMachineDialog::OnApply(wxCommandEvent& event) {
    if (ValidateInput()) {
        SaveMachineData();
        
        // Save to MachineConfigManager
        if (m_isNewMachine) {
            MachineConfigManager::Instance().AddMachine(m_config);
            m_isNewMachine = false; // Now it exists
        } else {
            MachineConfigManager::Instance().UpdateMachine(m_machineId, m_config);
        }
        
        wxMessageBox("Settings have been applied successfully.", "Settings Applied", wxOK | wxICON_INFORMATION);
    }
}

void EnhancedEditMachineDialog::OnQueryMachine(wxCommandEvent& event) {
    QueryMachineCapabilities();
}

void EnhancedEditMachineDialog::OnAutoDetectKinematics(wxCommandEvent& event) {
    // This would require the machine to be connected and have valid capabilities
    if (!m_config.capabilities.capabilitiesValid) {
        wxMessageBox("Please query machine capabilities first before auto-detecting kinematics.", 
                     "No Capabilities Data", wxOK | wxICON_WARNING);
        return;
    }
    
    // Use the kinematics detection logic
    std::string detectedKinematics = MachineConfigManager::Instance().DetectKinematics(
        m_config.capabilities.grblSettings, 
        m_config.capabilities.systemInfo
    );
    
    // Update the machine configuration
    m_config.capabilities.kinematics = detectedKinematics;
    
    // Auto-configure homing based on detected kinematics
    MachineConfigManager::Instance().AutoConfigureHoming(m_config.id, detectedKinematics);
    m_config = MachineConfigManager::Instance().GetMachine(m_config.id);
    
    // Update UI
    LoadMachineData();
    UpdateUI();
    
    wxMessageBox(wxString::Format("Detected kinematics: %s\nHoming sequence has been automatically configured.", 
                                  detectedKinematics), "Auto-Detection Complete", wxOK | wxICON_INFORMATION);
}
