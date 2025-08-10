/**
 * gui/MainFrame.cpp
 * Main application window implementation (minimal functional version)
 */

#include "MainFrame.h"
#include "WelcomeDialog.h"
#include "AboutDialog.h"
#include "DROPanel.h"
#include "JogPanel.h"
#include "MachineManagerPanel.h"
#include "ConsolePanel.h"
#include "GCodeEditor.h"
#include "MacroPanel.h"
#include "SVGViewer.h"
#include "SettingsPanel.h"
#include "core/SimpleLogger.h"
#include "core/Version.h"
#include "core/BuildCounter.h"
#include "core/ErrorHandler.h"
#include <wx/msgdlg.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <wx/notebook.h>

// Menu IDs
enum {
    ID_SHOW_WELCOME = wxID_HIGHEST + 1,
    ID_TEST_ERROR_HANDLER,
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
    EVT_MENU(ID_TEST_ERROR_HANDLER, MainFrame::OnTestErrorHandler)
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
    
    // Create all panels (simplified without AUI for now)
    CreatePanels();
    
    // Set up the initial layout (simplified)
    CreateDefaultLayout();
    
    // Update menu states
    UpdateMenuItems();
}

MainFrame::~MainFrame()
{
    // Cleanup (simplified without AUI)
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
    helpMenu->Append(ID_TEST_ERROR_HANDLER, "&Test Error Handler", "Test the error handling system");
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

void MainFrame::OnTestErrorHandler(wxCommandEvent& WXUNUSED(event))
{
    // Test different types of errors
    wxString choice[] = {
        "Test Error Dialog",
        "Test Warning Dialog",
        "Test Info Dialog",
        "Test wxWidgets Assertion"
    };
    
    wxSingleChoiceDialog dialog(this, "Choose error type to test:", "Error Handler Test", 4, choice);
    
    if (dialog.ShowModal() == wxID_OK) {
        int selection = dialog.GetSelection();
        
        switch (selection) {
            case 0:
                ErrorHandler::Instance().ReportError(
                    "Test Error", 
                    "This is a test error message to demonstrate the error handling system.",
                    "This error was triggered by the user through the Help menu.\n\nAll details are copyable and the application continues to run normally.");
                break;
                
            case 1:
                ErrorHandler::Instance().ReportWarning(
                    "Test Warning", 
                    "This is a test warning message.",
                    "Warnings are used for non-critical issues that the user should be aware of.");
                break;
                
            case 2:
                ErrorHandler::Instance().ReportInfo(
                    "Test Information", 
                    "This is a test information message.",
                    "Information messages provide helpful details to the user.");
                break;
                
            case 3:
                // This will trigger a wxWidgets assertion which our handler will catch
                wxASSERT_MSG(false, "This is a test assertion to demonstrate assertion handling");
                break;
        }
    }
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

// Missing method implementations
void MainFrame::CreatePanels()
{
    // Initialize panel storage but don't create panels yet
    // They will be created as children of the notebook in CreateDefaultLayout
}

void MainFrame::CreateDefaultLayout()
{
    // Simplified layout using notebook for now (without AUI)
    wxNotebook* notebook = new wxNotebook(this, wxID_ANY);
    
    // Create panels with notebook as parent and add them to tabs
    try {
        // DROPanel needs ConnectionManager - use simplified version for now
        wxPanel* droPanel = new DROPanel(notebook, nullptr); // nullptr for ConnectionManager
        notebook->AddPage(droPanel, "Digital Readout");
        
        // JogPanel needs ConnectionManager
        wxPanel* jogPanel = new JogPanel(notebook, nullptr); // nullptr for ConnectionManager
        notebook->AddPage(jogPanel, "Jogging Controls");
        
        // MachineManagerPanel
        wxPanel* machinePanel = new MachineManagerPanel(notebook);
        notebook->AddPage(machinePanel, "Machine Manager");
        
        // ConsolePanel (our terminal)
        wxPanel* consolePanel = new ConsolePanel(notebook);
        notebook->AddPage(consolePanel, "Terminal Console");
        
        // GCodeEditor
        wxPanel* gcodePanel = new GCodeEditor(notebook);
        notebook->AddPage(gcodePanel, "G-code Editor");
        
        // MacroPanel
        wxPanel* macroPanel = new MacroPanel(notebook);
        notebook->AddPage(macroPanel, "Macros");
        
        // SVGViewer
        wxPanel* svgPanel = new SVGViewer(notebook);
        notebook->AddPage(svgPanel, "SVG Viewer");
        
    } catch (const std::exception& e) {
        // If panel creation fails, show a simple error panel
        wxPanel* errorPanel = new wxPanel(notebook, wxID_ANY);
        wxStaticText* errorText = new wxStaticText(errorPanel, wxID_ANY, 
            wxString::Format("Panel creation error: %s", e.what()));
        wxBoxSizer* errorSizer = new wxBoxSizer(wxVERTICAL);
        errorSizer->Add(errorText, 1, wxALL | wxCENTER, 20);
        errorPanel->SetSizer(errorSizer);
        notebook->AddPage(errorPanel, "Error");
    }
    
    // Use simple sizer
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(notebook, 1, wxEXPAND | wxALL, 5);
    SetSizer(sizer);
}

void MainFrame::UpdateMenuItems()
{
    // Update menu item check states based on panel visibility
    wxMenuBar* menuBar = GetMenuBar();
    if (menuBar) {
        // For now, just set all panels as visible
        // TODO: Update based on actual panel visibility when implemented
        menuBar->Check(ID_WINDOW_DRO, true);
        menuBar->Check(ID_WINDOW_JOG, true);
        menuBar->Check(ID_WINDOW_MACHINE_MANAGER, true);
        menuBar->Check(ID_WINDOW_GCODE_EDITOR, true);
        menuBar->Check(ID_WINDOW_CONSOLE, true);
        menuBar->Check(ID_WINDOW_MACRO, true);
        menuBar->Check(ID_WINDOW_SVG_VIEWER, true);
        menuBar->Check(ID_WINDOW_SETTINGS, false); // Hidden by default
    }
}
