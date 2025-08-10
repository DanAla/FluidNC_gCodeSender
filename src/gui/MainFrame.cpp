/**
 * gui/MainFrame.cpp
 * Main application window implementation (minimal functional version)
 */

#include "MainFrame.h"
#include "WelcomeDialog.h"
#include "AboutDialog.h"
#include "core/SimpleLogger.h"
#include "core/Version.h"
#include "core/BuildCounter.h"
#include <wx/msgdlg.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>

// Menu IDs
enum {
    ID_SHOW_WELCOME = wxID_HIGHEST + 1,
    // Window menu items
    ID_WINDOW_DRO,
    ID_WINDOW_JOG,
    ID_WINDOW_SETTINGS,
    ID_WINDOW_MACHINE_MANAGER,
    ID_WINDOW_SVG_VIEWER,
    ID_WINDOW_GCODE_EDITOR,
    ID_WINDOW_MACRO,
    ID_WINDOW_CONSOLE,
    ID_WINDOW_RESET_LAYOUT
};

// Event table
wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(wxID_EXIT, MainFrame::OnExit)
    EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
    EVT_MENU(ID_SHOW_WELCOME, MainFrame::OnShowWelcome)
    // Window menu events
    EVT_MENU(ID_WINDOW_DRO, MainFrame::OnWindowDRO)
    EVT_MENU(ID_WINDOW_JOG, MainFrame::OnWindowJog)
    EVT_MENU(ID_WINDOW_SETTINGS, MainFrame::OnWindowSettings)
    EVT_MENU(ID_WINDOW_MACHINE_MANAGER, MainFrame::OnWindowMachineManager)
    EVT_MENU(ID_WINDOW_SVG_VIEWER, MainFrame::OnWindowSVGViewer)
    EVT_MENU(ID_WINDOW_GCODE_EDITOR, MainFrame::OnWindowGCodeEditor)
    EVT_MENU(ID_WINDOW_MACRO, MainFrame::OnWindowMacro)
    EVT_MENU(ID_WINDOW_CONSOLE, MainFrame::OnWindowConsole)
    EVT_MENU(ID_WINDOW_RESET_LAYOUT, MainFrame::OnWindowResetLayout)
    EVT_CLOSE(MainFrame::OnClose)
wxEND_EVENT_TABLE()

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, FluidNC::Version::GetFullVersionString(), wxDefaultPosition, wxSize(1200, 800))
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
    
    std::string welcomeMessage = std::string(FluidNC::Version::APP_NAME) + "\n\n" +
                                "Professional CNC Control Application\n\n" +
                                "Version " + FluidNC::Version::VERSION_STRING_STR + "\n" +
                                "Built: " + FluidNC::Version::BUILD_INFO;
    
    wxStaticText* welcomeText = new wxStaticText(centerPanel, wxID_ANY, 
        wxString::FromUTF8(welcomeMessage),
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
    
    // Window menu
    wxMenu* windowMenu = new wxMenu();
    windowMenu->AppendCheckItem(ID_WINDOW_DRO, "&DRO Panel\tCtrl+1", "Show/Hide Digital Readout panel");
    windowMenu->AppendCheckItem(ID_WINDOW_JOG, "&Jogging Panel\tCtrl+2", "Show/Hide Jogging control panel");
    windowMenu->AppendCheckItem(ID_WINDOW_MACHINE_MANAGER, "&Machine Manager\tCtrl+3", "Show/Hide Machine Manager panel");
    windowMenu->AppendSeparator();
    windowMenu->AppendCheckItem(ID_WINDOW_GCODE_EDITOR, "&G-code Editor\tCtrl+4", "Show/Hide G-code Editor panel");
    windowMenu->AppendCheckItem(ID_WINDOW_SVG_VIEWER, "&SVG Viewer\tCtrl+5", "Show/Hide SVG Viewer panel");
    windowMenu->AppendCheckItem(ID_WINDOW_MACRO, "&Macro Panel\tCtrl+6", "Show/Hide Macro panel");
    windowMenu->AppendSeparator();
    windowMenu->AppendCheckItem(ID_WINDOW_CONSOLE, "&Console\tCtrl+7", "Show/Hide Console panel");
    windowMenu->AppendCheckItem(ID_WINDOW_SETTINGS, "&Settings\tCtrl+8", "Show/Hide Settings panel");
    windowMenu->AppendSeparator();
    windowMenu->Append(ID_WINDOW_RESET_LAYOUT, "&Reset Layout\tCtrl+R", "Reset all panels to default layout");
    menuBar->Append(windowMenu, "&Window");
    
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
    AboutDialog dialog(this);
    dialog.ShowModal();
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

// Window menu event handlers
void MainFrame::OnWindowDRO(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle DRO Panel");
    wxMessageBox("DRO Panel toggle requested\n\nThis will show/hide the Digital Readout panel when implemented.", 
                 "DRO Panel", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnWindowJog(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle Jogging Panel");
    wxMessageBox("Jogging Panel toggle requested\n\nThis will show/hide the Jogging control panel when implemented.", 
                 "Jogging Panel", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnWindowSettings(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle Settings Panel");
    wxMessageBox("Settings Panel toggle requested\n\nThis will show/hide the Settings panel when implemented.", 
                 "Settings Panel", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnWindowMachineManager(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle Machine Manager Panel");
    wxMessageBox("Machine Manager Panel toggle requested\n\nThis will show/hide the Machine Manager panel when implemented.", 
                 "Machine Manager Panel", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnWindowSVGViewer(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle SVG Viewer Panel");
    wxMessageBox("SVG Viewer Panel toggle requested\n\nThis will show/hide the SVG Viewer panel when implemented.", 
                 "SVG Viewer Panel", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnWindowGCodeEditor(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle G-code Editor Panel");
    wxMessageBox("G-code Editor Panel toggle requested\n\nThis will show/hide the G-code Editor panel when implemented.", 
                 "G-code Editor Panel", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnWindowMacro(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle Macro Panel");
    wxMessageBox("Macro Panel toggle requested\n\nThis will show/hide the Macro panel when implemented.", 
                 "Macro Panel", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnWindowConsole(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle Console Panel");
    wxMessageBox("Console Panel toggle requested\n\nThis will show/hide the Console panel when implemented.", 
                 "Console Panel", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnWindowResetLayout(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Reset Layout");
    wxMessageBox("Reset Layout requested\n\nThis will reset all panels to their default positions when implemented.", 
                 "Reset Layout", wxOK | wxICON_INFORMATION, this);
}
