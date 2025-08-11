/**
 * gui/DROPanel.cpp
 * DRO Panel implementation with dummy content
 */

#include "DROPanel.h"
#include "NotificationSystem.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/timer.h>
// Control IDs
enum {
    ID_MACHINE_CHOICE = wxID_HIGHEST + 1100,
    ID_SEND_COMMAND,
    ID_COMMAND_INPUT,
    ID_HOME_ALL,
    ID_HOME_X,
    ID_HOME_Y,
    ID_HOME_Z,
    ID_ZERO_WORK,
    ID_ZERO_ALL,
    ID_SET_WORK,
    ID_SPINDLE_ON,
    ID_SPINDLE_OFF,
    ID_COOLANT_ON,
    ID_COOLANT_OFF,
    ID_FEED_HOLD,
    ID_RESUME,
    ID_RESET
};

wxBEGIN_EVENT_TABLE(DROPanel, wxPanel)
    EVT_CHOICE(ID_MACHINE_CHOICE, DROPanel::OnMachineChanged)
    EVT_BUTTON(ID_SEND_COMMAND, DROPanel::OnSendCommand)
    EVT_TEXT_ENTER(ID_COMMAND_INPUT, DROPanel::OnCommandEnter)
    EVT_BUTTON(ID_HOME_ALL, DROPanel::OnQuickCommand)
    EVT_BUTTON(ID_HOME_X, DROPanel::OnQuickCommand)
    EVT_BUTTON(ID_HOME_Y, DROPanel::OnQuickCommand)
    EVT_BUTTON(ID_HOME_Z, DROPanel::OnQuickCommand)
    EVT_BUTTON(ID_ZERO_WORK, DROPanel::OnZeroWork)
    EVT_BUTTON(ID_ZERO_ALL, DROPanel::OnZeroAll)
    EVT_BUTTON(ID_SET_WORK, DROPanel::OnSetWork)
    EVT_BUTTON(ID_SPINDLE_ON, DROPanel::OnQuickCommand)
    EVT_BUTTON(ID_SPINDLE_OFF, DROPanel::OnQuickCommand)
    EVT_BUTTON(ID_COOLANT_ON, DROPanel::OnQuickCommand)
    EVT_BUTTON(ID_COOLANT_OFF, DROPanel::OnQuickCommand)
    EVT_BUTTON(ID_FEED_HOLD, DROPanel::OnQuickCommand)
    EVT_BUTTON(ID_RESUME, DROPanel::OnQuickCommand)
    EVT_BUTTON(ID_RESET, DROPanel::OnQuickCommand)
wxEND_EVENT_TABLE()

DROPanel::DROPanel(wxWindow* parent, ConnectionManager* connectionManager)
    : wxPanel(parent), m_connectionManager(connectionManager)
{
    CreateControls();
    
    // Initialize with dummy data
    m_activeMachine = "CNC Router";
    UpdateCoordinateDisplay();
    UpdateMachineStatusDisplay();
}

void DROPanel::CreateControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    wxStaticText* title = new wxStaticText(this, wxID_ANY, "Digital Readout (DRO)");
    title->SetFont(title->GetFont().Scale(1.2).Bold());
    mainSizer->Add(title, 0, wxALL | wxCENTER, 5);
    
    CreateDRODisplay();
    CreateCommandInterface();
    CreateQuickCommands();
    
    mainSizer->Add(m_droSizer, 1, wxALL | wxEXPAND, 5);
    mainSizer->Add(m_commandSizer, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(m_quickSizer, 0, wxALL | wxEXPAND, 5);
    
    SetSizer(mainSizer);
}

void DROPanel::CreateDRODisplay()
{
    m_droSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Machine Position");
    
    // Machine selection
    wxBoxSizer* machineSizer = new wxBoxSizer(wxHORIZONTAL);
    m_machineLabel = new wxStaticText(this, wxID_ANY, "Machine:");
    m_machineChoice = new wxChoice(this, ID_MACHINE_CHOICE);
    m_machineChoice->Append("CNC Router");
    m_machineChoice->Append("Laser Engraver");
    m_machineChoice->Append("3D Printer");
    m_machineChoice->SetSelection(0);
    
    m_connectionStatus = new wxStaticText(this, wxID_ANY, "Disconnected");
    m_connectionStatus->SetForegroundColour(*wxRED);
    
    machineSizer->Add(m_machineLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    machineSizer->Add(m_machineChoice, 1, wxEXPAND | wxRIGHT, 10);
    machineSizer->Add(m_connectionStatus, 0, wxALIGN_CENTER_VERTICAL);
    
    m_droSizer->Add(machineSizer, 0, wxALL | wxEXPAND, 5);
    
    // Position display
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(5, 5, 5, 10);
    gridSizer->AddGrowableCol(1, 1);
    gridSizer->AddGrowableCol(2, 1);
    gridSizer->AddGrowableCol(3, 1);
    gridSizer->AddGrowableCol(4, 1);
    
    // Headers
    gridSizer->Add(new wxStaticText(this, wxID_ANY, ""), 0);
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "X"), 0, wxALIGN_CENTER);
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Y"), 0, wxALIGN_CENTER);
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Z"), 0, wxALIGN_CENTER);
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "A"), 0, wxALIGN_CENTER);
    
    // Machine Position
    m_mposLabel = new wxStaticText(this, wxID_ANY, "MPos:");
    m_mposLabel->SetFont(m_mposLabel->GetFont().Bold());
    gridSizer->Add(m_mposLabel, 0, wxALIGN_CENTER_VERTICAL);
    
    m_mposX = new wxStaticText(this, wxID_ANY, "0.000", wxDefaultPosition, wxSize(80, -1), wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
    m_mposY = new wxStaticText(this, wxID_ANY, "0.000", wxDefaultPosition, wxSize(80, -1), wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
    m_mposZ = new wxStaticText(this, wxID_ANY, "0.000", wxDefaultPosition, wxSize(80, -1), wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
    m_mposA = new wxStaticText(this, wxID_ANY, "0.000", wxDefaultPosition, wxSize(80, -1), wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
    
    wxFont monoFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    m_mposX->SetFont(monoFont);
    m_mposY->SetFont(monoFont);
    m_mposZ->SetFont(monoFont);
    m_mposA->SetFont(monoFont);
    
    gridSizer->Add(m_mposX, 1, wxEXPAND);
    gridSizer->Add(m_mposY, 1, wxEXPAND);
    gridSizer->Add(m_mposZ, 1, wxEXPAND);
    gridSizer->Add(m_mposA, 1, wxEXPAND);
    
    // Work Position
    m_wposLabel = new wxStaticText(this, wxID_ANY, "WPos:");
    m_wposLabel->SetFont(m_wposLabel->GetFont().Bold());
    gridSizer->Add(m_wposLabel, 0, wxALIGN_CENTER_VERTICAL);
    
    m_wposX = new wxStaticText(this, wxID_ANY, "0.000", wxDefaultPosition, wxSize(80, -1), wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
    m_wposY = new wxStaticText(this, wxID_ANY, "0.000", wxDefaultPosition, wxSize(80, -1), wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
    m_wposZ = new wxStaticText(this, wxID_ANY, "5.000", wxDefaultPosition, wxSize(80, -1), wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
    m_wposA = new wxStaticText(this, wxID_ANY, "0.000", wxDefaultPosition, wxSize(80, -1), wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
    
    m_wposX->SetFont(monoFont);
    m_wposY->SetFont(monoFont);
    m_wposZ->SetFont(monoFont);
    m_wposA->SetFont(monoFont);
    
    gridSizer->Add(m_wposX, 1, wxEXPAND);
    gridSizer->Add(m_wposY, 1, wxEXPAND);
    gridSizer->Add(m_wposZ, 1, wxEXPAND);
    gridSizer->Add(m_wposA, 1, wxEXPAND);
    
    m_droSizer->Add(gridSizer, 0, wxALL | wxEXPAND, 5);
    
    // Status display
    wxBoxSizer* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_statusLabel = new wxStaticText(this, wxID_ANY, "Status:");
    m_statusLabel->SetFont(m_statusLabel->GetFont().Bold());
    m_machineState = new wxStaticText(this, wxID_ANY, "Idle");
    m_machineState->SetForegroundColour(*wxGREEN);
    
    m_feedRate = new wxStaticText(this, wxID_ANY, "Feed: 0 mm/min");
    m_spindleSpeed = new wxStaticText(this, wxID_ANY, "Spindle: 0 RPM");
    m_coordinateSystem = new wxStaticText(this, wxID_ANY, "G54");
    
    statusSizer->Add(m_statusLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    statusSizer->Add(m_machineState, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 15);
    statusSizer->Add(m_feedRate, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 15);
    statusSizer->Add(m_spindleSpeed, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 15);
    statusSizer->Add(m_coordinateSystem, 0, wxALIGN_CENTER_VERTICAL);
    
    m_droSizer->Add(statusSizer, 0, wxALL, 5);
}

void DROPanel::CreateCommandInterface()
{
    m_commandSizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "Command Input");
    
    m_commandInput = new wxTextCtrl(this, ID_COMMAND_INPUT, wxEmptyString,
                                   wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_sendButton = new wxButton(this, ID_SEND_COMMAND, "Send");
    
    m_commandSizer->Add(m_commandInput, 1, wxEXPAND | wxRIGHT, 5);
    m_commandSizer->Add(m_sendButton, 0);
}

void DROPanel::CreateQuickCommands()
{
    m_quickSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Quick Commands");
    
    // Home commands
    wxStaticBoxSizer* homeSizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "Homing");
    
    m_homeAllBtn = new wxButton(this, ID_HOME_ALL, "Home All");
    m_homeXBtn = new wxButton(this, ID_HOME_X, "X");
    m_homeYBtn = new wxButton(this, ID_HOME_Y, "Y");
    m_homeZBtn = new wxButton(this, ID_HOME_Z, "Z");
    
    homeSizer->Add(m_homeAllBtn, 1, wxEXPAND | wxRIGHT, 5);
    homeSizer->Add(m_homeXBtn, 0, wxRIGHT, 3);
    homeSizer->Add(m_homeYBtn, 0, wxRIGHT, 3);
    homeSizer->Add(m_homeZBtn, 0);
    
    // Zero commands
    wxBoxSizer* zeroSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_zeroWorkBtn = new wxButton(this, ID_ZERO_WORK, "Zero Work");
    m_zeroAllBtn = new wxButton(this, ID_ZERO_ALL, "Zero All");
    m_setWorkBtn = new wxButton(this, ID_SET_WORK, "Set Work");
    
    zeroSizer->Add(m_zeroWorkBtn, 1, wxEXPAND | wxRIGHT, 5);
    zeroSizer->Add(m_zeroAllBtn, 1, wxEXPAND | wxRIGHT, 5);
    zeroSizer->Add(m_setWorkBtn, 1, wxEXPAND);
    
    // Control commands
    wxBoxSizer* controlSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_spindleOnBtn = new wxButton(this, ID_SPINDLE_ON, "Spindle On");
    m_spindleOffBtn = new wxButton(this, ID_SPINDLE_OFF, "Spindle Off");
    m_coolantOnBtn = new wxButton(this, ID_COOLANT_ON, "Coolant On");
    m_coolantOffBtn = new wxButton(this, ID_COOLANT_OFF, "Coolant Off");
    
    controlSizer->Add(m_spindleOnBtn, 1, wxEXPAND | wxRIGHT, 3);
    controlSizer->Add(m_spindleOffBtn, 1, wxEXPAND | wxRIGHT, 5);
    controlSizer->Add(m_coolantOnBtn, 1, wxEXPAND | wxRIGHT, 3);
    controlSizer->Add(m_coolantOffBtn, 1, wxEXPAND);
    
    // Emergency commands
    wxBoxSizer* emergencySizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_feedHoldBtn = new wxButton(this, ID_FEED_HOLD, "Feed Hold");
    m_resumeBtn = new wxButton(this, ID_RESUME, "Resume");
    m_resetBtn = new wxButton(this, ID_RESET, "Reset");
    
    m_feedHoldBtn->SetBackgroundColour(*wxYELLOW);
    m_resetBtn->SetBackgroundColour(wxColour(255, 200, 200));
    
    emergencySizer->Add(m_feedHoldBtn, 1, wxEXPAND | wxRIGHT, 5);
    emergencySizer->Add(m_resumeBtn, 1, wxEXPAND | wxRIGHT, 5);
    emergencySizer->Add(m_resetBtn, 1, wxEXPAND);
    
    m_quickSizer->Add(homeSizer, 0, wxALL | wxEXPAND, 3);
    m_quickSizer->Add(zeroSizer, 0, wxALL | wxEXPAND, 3);
    m_quickSizer->Add(controlSizer, 0, wxALL | wxEXPAND, 3);
    m_quickSizer->Add(emergencySizer, 0, wxALL | wxEXPAND, 3);
}

void DROPanel::UpdateCoordinateDisplay()
{
    // Real position data will be updated via callbacks from CommunicationManager
    // This method is now used for manual updates when needed
    // Position data comes from the connected machine, not simulated
}

void DROPanel::UpdateMachineStatusDisplay()
{
    // Real machine status will be updated via callbacks from CommunicationManager
    // This method is now used for manual updates when needed
    // Status data comes from the connected machine, not simulated
}

// Event handlers
void DROPanel::OnMachineChanged(wxCommandEvent& event)
{
    int selection = m_machineChoice->GetSelection();
    if (selection != wxNOT_FOUND) {
        m_activeMachine = m_machineChoice->GetString(selection).ToStdString();
        // Real connection status will be updated via CommunicationManager callbacks
        // No more simulated connection status changes
        Layout();
    }
}

void DROPanel::OnSendCommand(wxCommandEvent& WXUNUSED(event))
{
    wxString command = m_commandInput->GetValue().Trim();
    if (!command.IsEmpty()) {
        NotificationSystem::Instance().ShowInfo(
            "Command Sent", wxString::Format("Sending command: %s", command));
        m_commandInput->Clear();
    }
}

void DROPanel::OnCommandEnter(wxCommandEvent& event)
{
    OnSendCommand(event);
}

void DROPanel::OnQuickCommand(wxCommandEvent& event)
{
    wxString command;
    
    switch (event.GetId()) {
        case ID_HOME_ALL:
            command = "G28";
            break;
        case ID_HOME_X:
            command = "G28.2 X0";
            break;
        case ID_HOME_Y:
            command = "G28.2 Y0";
            break;
        case ID_HOME_Z:
            command = "G28.2 Z0";
            break;
        case ID_SPINDLE_ON:
            command = "M3 S1000";
            break;
        case ID_SPINDLE_OFF:
            command = "M5";
            break;
        case ID_COOLANT_ON:
            command = "M8";
            break;
        case ID_COOLANT_OFF:
            command = "M9";
            break;
        case ID_FEED_HOLD:
            command = "!";
            break;
        case ID_RESUME:
            command = "~";
            break;
        case ID_RESET:
            command = "Ctrl-X";
            break;
        default:
            return;
    }
    
    NotificationSystem::Instance().ShowInfo(
        "Quick Command", wxString::Format("Executing: %s", command));
}

void DROPanel::OnZeroWork(wxCommandEvent& WXUNUSED(event))
{
    int result = wxMessageBox("This will set the current position as the work zero.\n\nAre you sure?",
                             "Zero Work Position", wxYES_NO | wxICON_QUESTION, this);
    
    if (result == wxYES) {
        NotificationSystem::Instance().ShowSuccess("Zero Work", "Work position zeroed at current location.");
    }
}

void DROPanel::OnZeroAll(wxCommandEvent& WXUNUSED(event))
{
    int result = wxMessageBox("This will zero all work coordinates.\n\nAre you sure?",
                             "Zero All Coordinates", wxYES_NO | wxICON_QUESTION, this);
    
    if (result == wxYES) {
        NotificationSystem::Instance().ShowSuccess("Zero All", "All work coordinates have been zeroed.");
    }
}

void DROPanel::OnSetWork(wxCommandEvent& WXUNUSED(event))
{
    wxString value = wxGetTextFromUser("Enter new work coordinate value:", "Set Work Position", "0.000");
    if (!value.IsEmpty()) {
        NotificationSystem::Instance().ShowSuccess(
            "Set Work Position", wxString::Format("Work position set to: %s", value));
    }
}

// Public interface methods
void DROPanel::UpdateMachineStatus(const std::string& machineId, const MachineStatus& status) {
    // TODO: Update display with real machine status
    UpdateMachineStatusDisplay();
}

void DROPanel::SetActiveMachine(const std::string& machineId) {
    m_activeMachine = machineId;
    // Find and select machine in choice control
    for (unsigned int i = 0; i < m_machineChoice->GetCount(); ++i) {
        if (m_machineChoice->GetString(i).ToStdString() == machineId) {
            m_machineChoice->SetSelection(i);
            break;
        }
    }
}

void DROPanel::RefreshDisplay() {
    UpdateCoordinateDisplay();
    UpdateMachineStatusDisplay();
}
