/**
 * gui/ComprehensiveEditMachineDialog.cpp
 * Complete FluidNC/GRBL machine configuration dialog implementation
 * Auto-discovery system that populates ALL settings with one click
 */

#include "ComprehensiveEditMachineDialog.h"
#include "../core/SimpleLogger.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/datetime.h>
#include <thread>
#include <chrono>

wxBEGIN_EVENT_TABLE(ComprehensiveEditMachineDialog, wxDialog)
    EVT_BUTTON(wxID_OK, ComprehensiveEditMachineDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, ComprehensiveEditMachineDialog::OnCancel)
    EVT_BUTTON(wxID_APPLY, ComprehensiveEditMachineDialog::OnApply)
    EVT_BUTTON(ID_AUTO_DISCOVER, ComprehensiveEditMachineDialog::OnAutoDiscover)
    EVT_BUTTON(ID_TEST_CONNECTION, ComprehensiveEditMachineDialog::OnTestConnection)
    EVT_BUTTON(ID_RESET_DEFAULTS, ComprehensiveEditMachineDialog::OnResetToDefaults)
    EVT_BUTTON(ID_EXPORT_CONFIG, ComprehensiveEditMachineDialog::OnExportConfig)
    EVT_BUTTON(ID_IMPORT_CONFIG, ComprehensiveEditMachineDialog::OnImportConfig)
    EVT_BUTTON(ID_TEST_HOMING_SEQ, ComprehensiveEditMachineDialog::TestHoming)
    EVT_BUTTON(ID_TEST_PROBE_SEQ, ComprehensiveEditMachineDialog::TestProbe)
    EVT_BUTTON(ID_TEST_SPINDLE_CTRL, ComprehensiveEditMachineDialog::TestSpindle)
    EVT_BUTTON(ID_TEST_JOGGING, ComprehensiveEditMachineDialog::TestJogging)
    EVT_BUTTON(ID_TEST_LIMITS, ComprehensiveEditMachineDialog::TestLimits)
wxEND_EVENT_TABLE()

ComprehensiveEditMachineDialog::ComprehensiveEditMachineDialog(wxWindow* parent, const std::string& machineId, bool isNewMachine)
    : wxDialog(parent, wxID_ANY, isNewMachine ? "Add New Machine - Complete Configuration" : "Edit Machine - Complete Configuration", 
               wxDefaultPosition, wxSize(900, 700), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_machineId(machineId),
      m_isNewMachine(isNewMachine),
      m_discoveryInProgress(false),
      m_discoveryProgress(nullptr)
{
    // Initialize GRBL parameter definitions
    InitializeGRBLParameters();
    
    // Load existing machine data if editing
    if (!isNewMachine && !machineId.empty()) {
        m_config = MachineConfigManager::Instance().GetMachine(machineId);
        if (m_config.id.empty()) {
            m_isNewMachine = true;
            m_config = EnhancedMachineConfig{};
            m_config.id = machineId;
        }
    } else {
        m_config = EnhancedMachineConfig{};
        if (!machineId.empty()) {
            m_config.id = machineId;
        }
    }
    
    CreateNotebook();
    
    // Create main layout
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Add prominent auto-discovery section at top
    wxStaticBoxSizer* discoveryBox = new wxStaticBoxSizer(wxHORIZONTAL, this, "Machine Auto-Discovery");
    
    wxStaticText* discoveryText = new wxStaticText(this, wxID_ANY, 
        "Connect to your machine and click 'Auto-Discover' to automatically populate ALL settings:");
    discoveryText->Wrap(400);
    discoveryBox->Add(discoveryText, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    m_autoDiscoverBtn = new wxButton(this, ID_AUTO_DISCOVER, "🔍 Auto-Discover Machine", wxDefaultPosition, wxSize(200, 40));
    m_autoDiscoverBtn->SetFont(m_autoDiscoverBtn->GetFont().Scale(1.2).Bold());
    m_autoDiscoverBtn->SetBackgroundColour(wxColour(0, 120, 215)); // Blue
    m_autoDiscoverBtn->SetForegroundColour(*wxWHITE);
    m_autoDiscoverBtn->SetToolTip("Connect to machine and automatically discover ALL FluidNC/GRBL settings, capabilities, pin configurations, and features");
    discoveryBox->Add(m_autoDiscoverBtn, 0, wxALL, 5);
    
    mainSizer->Add(discoveryBox, 0, wxALL | wxEXPAND, 10);
    
    // Add notebook
    mainSizer->Add(m_notebook, 1, wxALL | wxEXPAND, 10);
    
    // Create button panel
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_resetBtn = new wxButton(this, ID_RESET_DEFAULTS, "Reset to Defaults");
    m_exportBtn = new wxButton(this, ID_EXPORT_CONFIG, "Export Config");
    m_importBtn = new wxButton(this, ID_IMPORT_CONFIG, "Import Config");
    
    buttonSizer->Add(m_resetBtn, 0, wxRIGHT, 5);
    buttonSizer->Add(m_exportBtn, 0, wxRIGHT, 5);
    buttonSizer->Add(m_importBtn, 0, wxRIGHT, 15);
    
    buttonSizer->AddStretchSpacer();
    
    m_okBtn = new wxButton(this, wxID_OK, "OK");
    m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    m_applyBtn = new wxButton(this, wxID_APPLY, "Apply");
    
    buttonSizer->Add(m_okBtn, 0, wxRIGHT, 5);
    buttonSizer->Add(m_cancelBtn, 0, wxRIGHT, 5);
    buttonSizer->Add(m_applyBtn, 0, 0);
    
    mainSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 10);
    
    SetSizer(mainSizer);
    
    // Load current settings
    LoadAllSettings();
}

ComprehensiveEditMachineDialog::~ComprehensiveEditMachineDialog() {
    if (m_discoveryProgress) {
        m_discoveryProgress->Destroy();
        m_discoveryProgress = nullptr;
    }
}

void ComprehensiveEditMachineDialog::CreateNotebook() {
    m_notebook = new wxNotebook(this, wxID_ANY);
    
    CreateBasicTab(m_notebook);
    CreateMotionTab(m_notebook);
    CreateHomingTab(m_notebook);
    CreateSpindleCoolantTab(m_notebook);
    CreateProbeTab(m_notebook);
    CreateSafetyLimitsTab(m_notebook);
    CreatePinConfigTab(m_notebook);
    CreateAdvancedTab(m_notebook);
    CreateSystemInfoTab(m_notebook);
    CreateTestingTab(m_notebook);
}

void ComprehensiveEditMachineDialog::CreateBasicTab(wxNotebook* notebook) {
    m_basicPanel = new wxPanel(notebook);
    notebook->AddPage(m_basicPanel, "Basic Settings", true);
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Machine Identity
    wxStaticBoxSizer* identityBox = new wxStaticBoxSizer(wxVERTICAL, m_basicPanel, "Machine Identity");
    
    wxFlexGridSizer* identityGrid = new wxFlexGridSizer(3, 2, 5, 10);
    identityGrid->AddGrowableCol(1, 1);
    
    identityGrid->Add(new wxStaticText(m_basicPanel, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
    m_nameText = new wxTextCtrl(m_basicPanel, wxID_ANY, "");
    identityGrid->Add(m_nameText, 1, wxEXPAND);
    
    identityGrid->Add(new wxStaticText(m_basicPanel, wxID_ANY, "Description:"), 0, wxALIGN_TOP);
    m_descriptionText = new wxTextCtrl(m_basicPanel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 60), wxTE_MULTILINE);
    identityGrid->Add(m_descriptionText, 1, wxEXPAND);
    
    identityGrid->Add(new wxStaticText(m_basicPanel, wxID_ANY, "Type:"), 0, wxALIGN_CENTER_VERTICAL);
    m_machineTypeChoice = new wxChoice(m_basicPanel, wxID_ANY);
    m_machineTypeChoice->Append("FluidNC");
    m_machineTypeChoice->Append("GRBL");
    m_machineTypeChoice->Append("Custom");
    m_machineTypeChoice->SetSelection(0);
    identityGrid->Add(m_machineTypeChoice, 1, wxEXPAND);
    
    identityBox->Add(identityGrid, 1, wxALL | wxEXPAND, 5);
    mainSizer->Add(identityBox, 0, wxALL | wxEXPAND, 5);
    
    // Connection Settings
    wxStaticBoxSizer* connectionBox = new wxStaticBoxSizer(wxVERTICAL, m_basicPanel, "Connection Settings");
    
    wxFlexGridSizer* connectionGrid = new wxFlexGridSizer(3, 2, 5, 10);
    connectionGrid->AddGrowableCol(1, 1);
    
    connectionGrid->Add(new wxStaticText(m_basicPanel, wxID_ANY, "Host/IP:"), 0, wxALIGN_CENTER_VERTICAL);
    m_hostText = new wxTextCtrl(m_basicPanel, wxID_ANY, "");
    connectionGrid->Add(m_hostText, 1, wxEXPAND);
    
    connectionGrid->Add(new wxStaticText(m_basicPanel, wxID_ANY, "Port:"), 0, wxALIGN_CENTER_VERTICAL);
    m_portSpinner = new wxSpinCtrl(m_basicPanel, wxID_ANY);
    m_portSpinner->SetRange(1, 65535);
    m_portSpinner->SetValue(23);
    connectionGrid->Add(m_portSpinner, 1, wxEXPAND);
    
    connectionGrid->Add(new wxStaticText(m_basicPanel, wxID_ANY, ""), 0); // Spacer
    m_autoConnectCheck = new wxCheckBox(m_basicPanel, wxID_ANY, "Auto-connect on startup");
    connectionGrid->Add(m_autoConnectCheck, 1, wxEXPAND);
    
    connectionBox->Add(connectionGrid, 1, wxALL | wxEXPAND, 5);
    
    // Connection testing
    wxBoxSizer* testSizer = new wxBoxSizer(wxHORIZONTAL);
    m_testConnectionBtn = new wxButton(m_basicPanel, ID_TEST_CONNECTION, "Test Connection");
    testSizer->AddStretchSpacer();
    testSizer->Add(m_testConnectionBtn, 0, wxALL, 5);
    
    connectionBox->Add(testSizer, 0, wxEXPAND);
    mainSizer->Add(connectionBox, 0, wxALL | wxEXPAND, 5);
    
    mainSizer->AddStretchSpacer(1);
    m_basicPanel->SetSizer(mainSizer);
}

void ComprehensiveEditMachineDialog::CreateMotionTab(wxNotebook* notebook) {
    m_motionPanel = new wxPanel(notebook);
    notebook->AddPage(m_motionPanel, "Motion Settings");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Steps per mm ($$100-103)
    wxStaticBoxSizer* stepsBox = new wxStaticBoxSizer(wxVERTICAL, m_motionPanel, "Steps per MM ($$100-103)");
    wxFlexGridSizer* stepsGrid = new wxFlexGridSizer(4, 2, 5, 10);
    stepsGrid->AddGrowableCol(1, 1);
    
    stepsGrid->Add(new wxStaticText(m_motionPanel, wxID_ANY, "X-Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    m_stepsPerMM_X = new wxSpinCtrlDouble(m_motionPanel, wxID_ANY);
    m_stepsPerMM_X->SetRange(0.1, 10000.0);
    m_stepsPerMM_X->SetValue(80.0);
    m_stepsPerMM_X->SetIncrement(0.1);
    m_stepsPerMM_X->SetDigits(3);
    stepsGrid->Add(m_stepsPerMM_X, 1, wxEXPAND);
    
    stepsGrid->Add(new wxStaticText(m_motionPanel, wxID_ANY, "Y-Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    m_stepsPerMM_Y = new wxSpinCtrlDouble(m_motionPanel, wxID_ANY);
    m_stepsPerMM_Y->SetRange(0.1, 10000.0);
    m_stepsPerMM_Y->SetValue(80.0);
    m_stepsPerMM_Y->SetIncrement(0.1);
    m_stepsPerMM_Y->SetDigits(3);
    stepsGrid->Add(m_stepsPerMM_Y, 1, wxEXPAND);
    
    stepsGrid->Add(new wxStaticText(m_motionPanel, wxID_ANY, "Z-Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    m_stepsPerMM_Z = new wxSpinCtrlDouble(m_motionPanel, wxID_ANY);
    m_stepsPerMM_Z->SetRange(0.1, 10000.0);
    m_stepsPerMM_Z->SetValue(400.0);
    m_stepsPerMM_Z->SetIncrement(0.1);
    m_stepsPerMM_Z->SetDigits(3);
    stepsGrid->Add(m_stepsPerMM_Z, 1, wxEXPAND);
    
    stepsGrid->Add(new wxStaticText(m_motionPanel, wxID_ANY, "A-Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    m_stepsPerMM_A = new wxSpinCtrlDouble(m_motionPanel, wxID_ANY);
    m_stepsPerMM_A->SetRange(0.1, 10000.0);
    m_stepsPerMM_A->SetValue(80.0);
    m_stepsPerMM_A->SetIncrement(0.1);
    m_stepsPerMM_A->SetDigits(3);
    stepsGrid->Add(m_stepsPerMM_A, 1, wxEXPAND);
    
    stepsBox->Add(stepsGrid, 1, wxALL | wxEXPAND, 5);
    mainSizer->Add(stepsBox, 0, wxALL | wxEXPAND, 5);
    
    // Max Feed Rates ($$110-113)
    wxStaticBoxSizer* feedBox = new wxStaticBoxSizer(wxVERTICAL, m_motionPanel, "Max Feed Rates mm/min ($$110-113)");
    wxFlexGridSizer* feedGrid = new wxFlexGridSizer(4, 2, 5, 10);
    feedGrid->AddGrowableCol(1, 1);
    
    feedGrid->Add(new wxStaticText(m_motionPanel, wxID_ANY, "X-Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    m_maxRate_X = new wxSpinCtrlDouble(m_motionPanel, wxID_ANY);
    m_maxRate_X->SetRange(1.0, 50000.0);
    m_maxRate_X->SetValue(3000.0);
    m_maxRate_X->SetIncrement(100.0);
    feedGrid->Add(m_maxRate_X, 1, wxEXPAND);
    
    feedGrid->Add(new wxStaticText(m_motionPanel, wxID_ANY, "Y-Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    m_maxRate_Y = new wxSpinCtrlDouble(m_motionPanel, wxID_ANY);
    m_maxRate_Y->SetRange(1.0, 50000.0);
    m_maxRate_Y->SetValue(3000.0);
    m_maxRate_Y->SetIncrement(100.0);
    feedGrid->Add(m_maxRate_Y, 1, wxEXPAND);
    
    feedGrid->Add(new wxStaticText(m_motionPanel, wxID_ANY, "Z-Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    m_maxRate_Z = new wxSpinCtrlDouble(m_motionPanel, wxID_ANY);
    m_maxRate_Z->SetRange(1.0, 50000.0);
    m_maxRate_Z->SetValue(500.0);
    m_maxRate_Z->SetIncrement(100.0);
    feedGrid->Add(m_maxRate_Z, 1, wxEXPAND);
    
    feedGrid->Add(new wxStaticText(m_motionPanel, wxID_ANY, "A-Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    m_maxRate_A = new wxSpinCtrlDouble(m_motionPanel, wxID_ANY);
    m_maxRate_A->SetRange(1.0, 50000.0);
    m_maxRate_A->SetValue(3000.0);
    m_maxRate_A->SetIncrement(100.0);
    feedGrid->Add(m_maxRate_A, 1, wxEXPAND);
    
    feedBox->Add(feedGrid, 1, wxALL | wxEXPAND, 5);
    mainSizer->Add(feedBox, 0, wxALL | wxEXPAND, 5);
    
    // Max Travel ($$130-133)
    wxStaticBoxSizer* travelBox = new wxStaticBoxSizer(wxVERTICAL, m_motionPanel, "Max Travel mm ($$130-133)");
    wxFlexGridSizer* travelGrid = new wxFlexGridSizer(4, 2, 5, 10);
    travelGrid->AddGrowableCol(1, 1);
    
    travelGrid->Add(new wxStaticText(m_motionPanel, wxID_ANY, "X-Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    m_maxTravel_X = new wxSpinCtrlDouble(m_motionPanel, wxID_ANY);
    m_maxTravel_X->SetRange(1.0, 2000.0);
    m_maxTravel_X->SetValue(400.0);
    m_maxTravel_X->SetIncrement(1.0);
    travelGrid->Add(m_maxTravel_X, 1, wxEXPAND);
    
    travelGrid->Add(new wxStaticText(m_motionPanel, wxID_ANY, "Y-Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    m_maxTravel_Y = new wxSpinCtrlDouble(m_motionPanel, wxID_ANY);
    m_maxTravel_Y->SetRange(1.0, 2000.0);
    m_maxTravel_Y->SetValue(400.0);
    m_maxTravel_Y->SetIncrement(1.0);
    travelGrid->Add(m_maxTravel_Y, 1, wxEXPAND);
    
    travelGrid->Add(new wxStaticText(m_motionPanel, wxID_ANY, "Z-Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    m_maxTravel_Z = new wxSpinCtrlDouble(m_motionPanel, wxID_ANY);
    m_maxTravel_Z->SetRange(1.0, 500.0);
    m_maxTravel_Z->SetValue(100.0);
    m_maxTravel_Z->SetIncrement(1.0);
    travelGrid->Add(m_maxTravel_Z, 1, wxEXPAND);
    
    travelGrid->Add(new wxStaticText(m_motionPanel, wxID_ANY, "A-Axis:"), 0, wxALIGN_CENTER_VERTICAL);
    m_maxTravel_A = new wxSpinCtrlDouble(m_motionPanel, wxID_ANY);
    m_maxTravel_A->SetRange(1.0, 2000.0);
    m_maxTravel_A->SetValue(360.0);
    m_maxTravel_A->SetIncrement(1.0);
    travelGrid->Add(m_maxTravel_A, 1, wxEXPAND);
    
    travelBox->Add(travelGrid, 1, wxALL | wxEXPAND, 5);
    mainSizer->Add(travelBox, 0, wxALL | wxEXPAND, 5);
    
    mainSizer->AddStretchSpacer(1);
    m_motionPanel->SetSizer(mainSizer);
}

void ComprehensiveEditMachineDialog::CreateSystemInfoTab(wxNotebook* notebook) {
    m_systemInfoPanel = new wxPanel(notebook);
    notebook->AddPage(m_systemInfoPanel, "System Info & GRBL Settings");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // System Information
    wxStaticBoxSizer* sysInfoBox = new wxStaticBoxSizer(wxVERTICAL, m_systemInfoPanel, "System Information");
    
    wxFlexGridSizer* sysGrid = new wxFlexGridSizer(4, 2, 5, 10);
    sysGrid->AddGrowableCol(1, 1);
    
    sysGrid->Add(new wxStaticText(m_systemInfoPanel, wxID_ANY, "Firmware Version:"), 0, wxALIGN_CENTER_VERTICAL);
    m_firmwareVersion = new wxTextCtrl(m_systemInfoPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    sysGrid->Add(m_firmwareVersion, 1, wxEXPAND);
    
    sysGrid->Add(new wxStaticText(m_systemInfoPanel, wxID_ANY, "Build Date:"), 0, wxALIGN_CENTER_VERTICAL);
    m_buildDate = new wxTextCtrl(m_systemInfoPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    sysGrid->Add(m_buildDate, 1, wxEXPAND);
    
    sysGrid->Add(new wxStaticText(m_systemInfoPanel, wxID_ANY, "Build Options:"), 0, wxALIGN_CENTER_VERTICAL);
    m_buildOptions = new wxTextCtrl(m_systemInfoPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    sysGrid->Add(m_buildOptions, 1, wxEXPAND);
    
    sysGrid->Add(new wxStaticText(m_systemInfoPanel, wxID_ANY, "Capabilities:"), 0, wxALIGN_CENTER_VERTICAL);
    m_systemCapabilities = new wxTextCtrl(m_systemInfoPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    sysGrid->Add(m_systemCapabilities, 1, wxEXPAND);
    
    sysInfoBox->Add(sysGrid, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(sysInfoBox, 0, wxALL | wxEXPAND, 5);
    
    // GRBL Settings List
    wxStaticBoxSizer* grblBox = new wxStaticBoxSizer(wxVERTICAL, m_systemInfoPanel, "All GRBL Settings ($$)");
    
    m_grblSettingsList = new wxListCtrl(m_systemInfoPanel, ID_GRBL_SETTINGS_GRID, wxDefaultPosition, wxSize(-1, 200), 
                                        wxLC_REPORT | wxLC_SINGLE_SEL);
    m_grblSettingsList->AppendColumn("Parameter", wxLIST_FORMAT_LEFT, 80);
    m_grblSettingsList->AppendColumn("Value", wxLIST_FORMAT_LEFT, 100);
    m_grblSettingsList->AppendColumn("Description", wxLIST_FORMAT_LEFT, 300);
    m_grblSettingsList->AppendColumn("Unit", wxLIST_FORMAT_LEFT, 60);
    
    grblBox->Add(m_grblSettingsList, 1, wxALL | wxEXPAND, 5);
    mainSizer->Add(grblBox, 1, wxALL | wxEXPAND, 5);
    
    // Discovery Log
    wxStaticBoxSizer* logBox = new wxStaticBoxSizer(wxVERTICAL, m_systemInfoPanel, "Auto-Discovery Log");
    m_discoveryLog = new wxTextCtrl(m_systemInfoPanel, wxID_ANY, "Click 'Auto-Discover Machine' to populate all settings...", 
                                    wxDefaultPosition, wxSize(-1, 100), wxTE_MULTILINE | wxTE_READONLY);
    logBox->Add(m_discoveryLog, 1, wxALL | wxEXPAND, 5);
    mainSizer->Add(logBox, 0, wxALL | wxEXPAND, 5);
    
    m_systemInfoPanel->SetSizer(mainSizer);
}

// Initialize complete GRBL parameter definitions
void ComprehensiveEditMachineDialog::InitializeGRBLParameters() {
    m_grblParameters = {
        // Basic Settings (0-9)
        {0, "step_pulse_time", "Step pulse time in microseconds", "μs", 10.0f, 3.0f, 1000.0f, "Basic"},
        {1, "step_idle_delay", "Step idle delay in milliseconds", "ms", 25.0f, 0.0f, 255.0f, "Basic"},
        {2, "step_pulse_invert", "Step pulse invert mask", "mask", 0.0f, 0.0f, 255.0f, "Basic"},
        {3, "step_direction_invert", "Step direction invert mask", "mask", 0.0f, 0.0f, 255.0f, "Basic"},
        {4, "invert_step_enable", "Invert step enable pin", "bool", 0.0f, 0.0f, 1.0f, "Basic"},
        {5, "invert_limit_pins", "Invert limit pins", "bool", 0.0f, 0.0f, 1.0f, "Basic"},
        {6, "invert_probe_pin", "Invert probe pin", "bool", 0.0f, 0.0f, 1.0f, "Basic"},
        
        // General Settings (10-19)
        {10, "status_report", "Status report options", "mask", 1.0f, 0.0f, 3.0f, "General"},
        {11, "junction_deviation", "Junction deviation in mm", "mm", 0.010f, 0.001f, 0.200f, "General"},
        {12, "arc_tolerance", "Arc tolerance in mm", "mm", 0.002f, 0.001f, 0.100f, "General"},
        {13, "report_inches", "Report in inches instead of mm", "bool", 0.0f, 0.0f, 1.0f, "General"},
        
        // Safety & Limits (20-29)
        {20, "soft_limits", "Soft limits enable", "bool", 0.0f, 0.0f, 1.0f, "Safety"},
        {21, "hard_limits", "Hard limits enable", "bool", 0.0f, 0.0f, 1.0f, "Safety"},
        {22, "homing_cycle", "Homing cycle enable", "bool", 0.0f, 0.0f, 1.0f, "Homing"},
        {23, "homing_dir_invert", "Homing direction invert mask", "mask", 0.0f, 0.0f, 255.0f, "Homing"},
        {24, "homing_feed", "Homing locate feed rate", "mm/min", 25.0f, 1.0f, 10000.0f, "Homing"},
        {25, "homing_seek", "Homing search seek rate", "mm/min", 500.0f, 1.0f, 10000.0f, "Homing"},
        {26, "homing_debounce", "Homing switch debounce delay", "ms", 250.0f, 0.0f, 10000.0f, "Homing"},
        {27, "homing_pulloff", "Homing switch pull-off distance", "mm", 1.0f, 0.0f, 100.0f, "Homing"},
        
        // Motion Settings X-Axis (100-109)
        {100, "x_steps_per_mm", "X-axis steps per mm", "steps/mm", 80.0f, 0.1f, 10000.0f, "Motion"},
        {101, "y_steps_per_mm", "Y-axis steps per mm", "steps/mm", 80.0f, 0.1f, 10000.0f, "Motion"},
        {102, "z_steps_per_mm", "Z-axis steps per mm", "steps/mm", 400.0f, 0.1f, 10000.0f, "Motion"},
        {103, "a_steps_per_mm", "A-axis steps per mm", "steps/mm", 80.0f, 0.1f, 10000.0f, "Motion"},
        
        // Max Feed Rates (110-119)
        {110, "x_max_rate", "X-axis maximum feed rate", "mm/min", 3000.0f, 1.0f, 50000.0f, "Motion"},
        {111, "y_max_rate", "Y-axis maximum feed rate", "mm/min", 3000.0f, 1.0f, 50000.0f, "Motion"},
        {112, "z_max_rate", "Z-axis maximum feed rate", "mm/min", 500.0f, 1.0f, 50000.0f, "Motion"},
        {113, "a_max_rate", "A-axis maximum feed rate", "mm/min", 3000.0f, 1.0f, 50000.0f, "Motion"},
        
        // Acceleration (120-129)
        {120, "x_acceleration", "X-axis acceleration", "mm/sec²", 30.0f, 1.0f, 1000.0f, "Motion"},
        {121, "y_acceleration", "Y-axis acceleration", "mm/sec²", 30.0f, 1.0f, 1000.0f, "Motion"},
        {122, "z_acceleration", "Z-axis acceleration", "mm/sec²", 10.0f, 1.0f, 1000.0f, "Motion"},
        {123, "a_acceleration", "A-axis acceleration", "mm/sec²", 30.0f, 1.0f, 1000.0f, "Motion"},
        
        // Max Travel (130-139)
        {130, "x_max_travel", "X-axis maximum travel", "mm", 400.0f, 1.0f, 2000.0f, "Motion"},
        {131, "y_max_travel", "Y-axis maximum travel", "mm", 400.0f, 1.0f, 2000.0f, "Motion"},
        {132, "z_max_travel", "Z-axis maximum travel", "mm", 100.0f, 1.0f, 500.0f, "Motion"},
        {133, "a_max_travel", "A-axis maximum travel", "mm", 360.0f, 1.0f, 2000.0f, "Motion"},
        
        // FluidNC Extended Settings (400+)
        {400, "kinematics_type", "Kinematics type (0=Cartesian, 1=CoreXY)", "type", 0.0f, 0.0f, 10.0f, "FluidNC"},
        {401, "spindle_max_rpm", "Spindle maximum RPM", "rpm", 24000.0f, 1.0f, 50000.0f, "Spindle"},
        {402, "spindle_min_rpm", "Spindle minimum RPM", "rpm", 0.0f, 0.0f, 10000.0f, "Spindle"},
        {403, "spindle_pwm_freq", "Spindle PWM frequency", "Hz", 5000.0f, 1.0f, 50000.0f, "Spindle"}
    };
}

// THE BIG AUTO-DISCOVERY FUNCTION!
void ComprehensiveEditMachineDialog::OnAutoDiscover(wxCommandEvent& event) {
    if (m_discoveryInProgress) {
        wxMessageBox("Auto-discovery is already in progress!", "Discovery In Progress", wxOK | wxICON_INFORMATION);
        return;
    }
    
    // Validate connection settings
    if (m_hostText->GetValue().Trim().IsEmpty()) {
        wxMessageBox("Please enter a host address before starting auto-discovery.", "Missing Host", wxOK | wxICON_WARNING);
        m_notebook->SetSelection(0); // Switch to basic tab
        m_hostText->SetFocus();
        return;
    }
    
    StartAutoDiscovery();
}

void ComprehensiveEditMachineDialog::StartAutoDiscovery() {
    m_discoveryInProgress = true;
    m_autoDiscoverBtn->Enable(false);
    m_discoveryLog->Clear();
    m_discoveryLog->AppendText("🚀 Starting comprehensive machine auto-discovery...\n");
    
    // Show progress dialog
    m_discoveryProgress = new wxProgressDialog(
        "Auto-Discovering Machine Configuration",
        "Connecting to machine...",
        100, this, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT
    );
    
    // Start discovery in separate thread
    std::thread discoveryThread([this]() {
        try {
            // Phase 1: Connect to machine
            CallAfter([this]() {
                OnDiscoveryProgress("Connecting to machine...", 10);
                m_discoveryLog->AppendText("📡 Connecting to " + m_hostText->GetValue().ToStdString() + ":" + std::to_string(m_portSpinner->GetValue()) + "\n");
            });
            
            std::string tempId = "temp_discovery_" + std::to_string(std::time(nullptr));
            bool connected = CommunicationManager::Instance().ConnectMachine(
                tempId, m_hostText->GetValue().ToStdString(), m_portSpinner->GetValue()
            );
            
            if (!connected) {
                CallAfter([this]() {
                    OnDiscoveryError("Failed to connect to machine. Check host and port settings.");
                });
                return;
            }
            
            CallAfter([this]() {
                m_discoveryLog->AppendText("✅ Connected successfully!\n");
            });
            
            // Phase 2: Discover system information
            CallAfter([this]() {
                OnDiscoveryProgress("Querying system information ($I)...", 20);
            });
            DiscoverSystemInfo();
            
            // Phase 3: Discover ALL GRBL settings
            CallAfter([this]() {
                OnDiscoveryProgress("Querying all GRBL settings ($$)...", 40);
            });
            DiscoverGRBLSettings();
            
            // Phase 4: Discover build information
            CallAfter([this]() {
                OnDiscoveryProgress("Querying build information ($I)...", 60);
            });
            DiscoverBuildInfo();
            
            // Phase 5: Detect kinematics and capabilities
            CallAfter([this]() {
                OnDiscoveryProgress("Detecting kinematics and capabilities...", 80);
            });
            DiscoverKinematics();
            
            // Phase 6: Auto-configure all settings
            CallAfter([this]() {
                OnDiscoveryProgress("Auto-configuring all settings...", 90);
            });
            AutoConfigureFromDiscovery();
            
            // Disconnect
            CommunicationManager::Instance().DisconnectMachine(tempId);
            
            CallAfter([this]() {
                OnDiscoveryProgress("Discovery complete!", 100);
                OnDiscoveryComplete();
            });
            
        } catch (const std::exception& e) {
            CallAfter([this, e]() {
                OnDiscoveryError("Discovery failed: " + std::string(e.what()));
            });
        }
    });
    discoveryThread.detach();
}
