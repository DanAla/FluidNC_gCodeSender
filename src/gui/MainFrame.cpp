/**
 * gui/MainFrame.cpp
 * Main application window implementation (minimal functional version)
 */

#include "MainFrame.h"
#include "WelcomeDialog.h"
#include <wx/msgdlg.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>

// Menu IDs
enum {
    ID_SHOW_WELCOME = wxID_HIGHEST + 1
};

// Event table
wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(wxID_EXIT, MainFrame::OnExit)
    EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
    EVT_MENU(ID_SHOW_WELCOME, MainFrame::OnShowWelcome)
    EVT_CLOSE(MainFrame::OnClose)
wxEND_EVENT_TABLE()

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "FluidNC gCode Sender", wxDefaultPosition, wxSize(1200, 800))
{
    // Create menu bar
    CreateMenuBar();
    
    // Create status bar
    wxFrame::CreateStatusBar(4);
    SetStatusText("Ready", 0);
    SetStatusText("No machine connected", 1);
    SetStatusText("Disconnected", 2);
    SetStatusText("Position: ---", 3);
    
    // Create a simple center panel
    wxPanel* centerPanel = new wxPanel(this, wxID_ANY);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* welcomeText = new wxStaticText(centerPanel, wxID_ANY, 
        "FluidNC gCode Sender\n\nProfessional CNC Control Application\n\nVersion 1.0.0",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    
    welcomeText->SetFont(welcomeText->GetFont().Scale(1.5));
    
    sizer->Add(welcomeText, 1, wxALL | wxEXPAND, 20);
    centerPanel->SetSizer(sizer);
    
    // Use simple sizer layout
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(centerPanel, 1, wxEXPAND);
    SetSizer(mainSizer);
}

MainFrame::~MainFrame()
{
    // Cleanup (AUI temporarily removed)
}

void MainFrame::CreateMenuBar()
{
    wxMenuBar* menuBar = new wxMenuBar();
    
    // File menu
    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(wxID_EXIT, "E&xit\tCtrl+Q", "Exit the application");
    menuBar->Append(fileMenu, "&File");
    
    // Help menu
    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(ID_SHOW_WELCOME, "Show &Welcome Dialog", "Show the welcome dialog again");
    helpMenu->AppendSeparator();
    helpMenu->Append(wxID_ABOUT, "&About\tF1", "Show about dialog");
    menuBar->Append(helpMenu, "&Help");
    
    SetMenuBar(menuBar);
}

void MainFrame::RestoreWindowGeometry()
{
    // Simplified - just use default positioning for now
    Center();
}

void MainFrame::SaveWindowGeometry()
{
    // Simplified - no state saving for now
}

// Event handlers
void MainFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

void MainFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox("FluidNC gCode Sender v1.0.0\n\n"
                 "Professional CNC Control Application\n"
                 "Built with C++ and wxWidgets\n\n"
                 "Supports multiple CNC machines via Telnet, USB, and UART",
                 "About FluidNC gCode Sender",
                 wxOK | wxICON_INFORMATION,
                 this);
}

void MainFrame::OnShowWelcome(wxCommandEvent& WXUNUSED(event))
{
    WelcomeDialog dialog(this);
    dialog.ShowModal();
}

void MainFrame::OnClose(wxCloseEvent& WXUNUSED(event))
{
    try {
        SaveWindowGeometry();
        // StateManager will handle final save in its destructor
    } catch (const std::exception& e) {
        // Ignore errors during shutdown
    }
    Destroy();
}

// Stub implementations for interface compliance
void MainFrame::ShowPanel(PanelID panelId, bool show) {
    // TODO: Implement panel management
}

void MainFrame::TogglePanelVisibility(PanelID panelId) {
    // TODO: Implement panel toggling
}

bool MainFrame::IsPanelVisible(PanelID panelId) const {
    // TODO: Implement panel visibility check
    return false;
}

void MainFrame::ResetLayout() {
    // TODO: Implement layout reset
}

void MainFrame::SaveCurrentLayout() {
    SaveWindowGeometry();
}

void MainFrame::LoadSavedLayout() {
    RestoreWindowGeometry();
}

void MainFrame::UpdateMachineStatus(const std::string& machineId, const std::string& status) {
    SetStatusText(wxString::Format("Machine: %s", status), 1);
}

void MainFrame::UpdateConnectionStatus(const std::string& machineId, bool connected) {
    SetStatusText(connected ? "Connected" : "Disconnected", 2);
}

void MainFrame::UpdateDRO(const std::string& machineId, const std::vector<float>& mpos, const std::vector<float>& wpos) {
    if (!mpos.empty() && mpos.size() >= 3) {
        SetStatusText(wxString::Format("X:%.3f Y:%.3f Z:%.3f", mpos[0], mpos[1], mpos[2]), 3);
    }
}
