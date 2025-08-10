/**
 * gui/MainFrame.cpp
 * Main application window implementation (simple notebook-based version)
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
#include "SettingsDialog.h"
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
    , m_notebook(nullptr)
{
    // Create menu bar
    CreateMenuBar();
    
    // Create status bar
    wxFrame::CreateStatusBar(4);
    SetStatusText("Ready", 0);
    SetStatusText("No machine connected", 1);
    SetStatusText("Disconnected", 2);
    SetStatusText("Position: ---", 3);
    
    // Create notebook-based layout
    CreatePanels();
    
    // Update menu states
    UpdateMenuItems();
}

MainFrame::~MainFrame()
{
    // No special cleanup needed for notebook
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

// Panel management implementations (simplified for notebook)
void MainFrame::ShowPanel(PanelID panelId, bool show) {
    PanelInfo* panelInfo = FindPanelInfo(panelId);
    if (panelInfo && m_notebook) {
        // Find the page in the notebook
        for (size_t i = 0; i < m_notebook->GetPageCount(); ++i) {
            if (m_notebook->GetPage(i) == panelInfo->panel) {
                if (show) {
                    m_notebook->SetSelection(i);
                } 
                // For notebook, we can't really hide individual pages
                // but we could remove/add them dynamically if needed
                UpdateMenuItems();
                return;
            }
        }
        
        // If showing and not found, add it
        if (show) {
            m_notebook->AddPage(panelInfo->panel, panelInfo->title, true);
            UpdateMenuItems();
        }
    }
}

void MainFrame::TogglePanelVisibility(PanelID panelId) {
    // For notebook, just switch to the panel
    ShowPanel(panelId, true);
}

bool MainFrame::IsPanelVisible(PanelID panelId) const {
    PanelInfo* panelInfo = const_cast<MainFrame*>(this)->FindPanelInfo(panelId);
    if (panelInfo && m_notebook) {
        // Check if panel is in notebook
        for (size_t i = 0; i < m_notebook->GetPageCount(); ++i) {
            if (m_notebook->GetPage(i) == panelInfo->panel) {
                return true;
            }
        }
    }
    return false;
}

void MainFrame::ResetLayout() {
    // For notebook, just reset to default tab order
    if (m_notebook) {
        int currentSelection = m_notebook->GetSelection();
        if (currentSelection != 0 && m_notebook->GetPageCount() > 0) {
            m_notebook->SetSelection(0);
        }
    }
    UpdateMenuItems();
}

void MainFrame::SaveCurrentLayout() {
    // Simplified - no state saving for now
}

void MainFrame::LoadSavedLayout() {
    // Simplified - no state loading for now
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
    LOG_INFO("Window menu: Show DRO Panel");
    ShowPanel(PANEL_DRO, true);
}

void MainFrame::OnWindowJog(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Show Jogging Panel");
    ShowPanel(PANEL_JOG, true);
}

void MainFrame::OnWindowSettings(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Settings Panel (modal dialog)");
    // Settings dialog currently disabled due to ConnectionManager dependency
    wxMessageBox(
        "Settings Dialog temporarily disabled.\n\n"
        "The Settings dialog requires ConnectionManager which is currently disabled.\n"
        "This will be re-enabled in a future build when ConnectionManager is activated.",
        "Settings Unavailable",
        wxOK | wxICON_INFORMATION,
        this
    );
}

void MainFrame::OnWindowMachineManager(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Show Machine Manager Panel");
    ShowPanel(PANEL_MACHINE_MANAGER, true);
}

void MainFrame::OnWindowSVGViewer(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Show SVG Viewer Panel");
    ShowPanel(PANEL_SVG_VIEWER, true);
}

void MainFrame::OnWindowGCodeEditor(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Show G-code Editor Panel");
    ShowPanel(PANEL_GCODE_EDITOR, true);
}

void MainFrame::OnWindowMacro(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Show Macro Panel");
    ShowPanel(PANEL_MACRO, true);
}

void MainFrame::OnWindowConsole(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Show Console Panel");
    ShowPanel(PANEL_CONSOLE, true);
}

void MainFrame::OnWindowResetLayout(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Reset Layout");
    ResetLayout();
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

// Notebook-based panel creation
void MainFrame::CreatePanels()
{
    // Create main notebook
    m_notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);
    
    // Clear any existing panels
    m_panels.clear();
    
    try {
        // Create panels as children of the notebook
        
        // G-Code Editor - first tab
        PanelInfo gcodeInfo;
        gcodeInfo.id = PANEL_GCODE_EDITOR;
        gcodeInfo.name = "gcode_editor";
        gcodeInfo.title = "G-code Editor";
        gcodeInfo.panel = new GCodeEditor(m_notebook);
        gcodeInfo.defaultVisible = true;
        m_panels.push_back(gcodeInfo);
        m_notebook->AddPage(gcodeInfo.panel, gcodeInfo.title, true);
        
        // DRO Panel - second tab
        PanelInfo droInfo;
        droInfo.id = PANEL_DRO;
        droInfo.name = "dro";
        droInfo.title = "Digital Readout";
        droInfo.panel = new DROPanel(m_notebook, nullptr); // nullptr for ConnectionManager
        droInfo.defaultVisible = true;
        m_panels.push_back(droInfo);
        m_notebook->AddPage(droInfo.panel, droInfo.title);
        
        // Jog Panel - third tab
        PanelInfo jogInfo;
        jogInfo.id = PANEL_JOG;
        jogInfo.name = "jog";
        jogInfo.title = "Jogging Controls";
        jogInfo.panel = new JogPanel(m_notebook, nullptr); // nullptr for ConnectionManager
        jogInfo.defaultVisible = true;
        m_panels.push_back(jogInfo);
        m_notebook->AddPage(jogInfo.panel, jogInfo.title);
        
        // Console Panel
        PanelInfo consoleInfo;
        consoleInfo.id = PANEL_CONSOLE;
        consoleInfo.name = "console";
        consoleInfo.title = "Terminal Console";
        consoleInfo.panel = new ConsolePanel(m_notebook);
        consoleInfo.defaultVisible = true;
        m_panels.push_back(consoleInfo);
        m_notebook->AddPage(consoleInfo.panel, consoleInfo.title);
        
        // Machine Manager Panel
        PanelInfo machineInfo;
        machineInfo.id = PANEL_MACHINE_MANAGER;
        machineInfo.name = "machine_manager";
        machineInfo.title = "Machine Manager";
        machineInfo.panel = new MachineManagerPanel(m_notebook);
        machineInfo.defaultVisible = true;
        m_panels.push_back(machineInfo);
        m_notebook->AddPage(machineInfo.panel, machineInfo.title);
        
        // Macro Panel
        PanelInfo macroInfo;
        macroInfo.id = PANEL_MACRO;
        macroInfo.name = "macro";
        macroInfo.title = "Macro Panel";
        macroInfo.panel = new MacroPanel(m_notebook);
        macroInfo.defaultVisible = true;
        m_panels.push_back(macroInfo);
        m_notebook->AddPage(macroInfo.panel, macroInfo.title);
        
        // SVG Viewer - initially hidden
        PanelInfo svgInfo;
        svgInfo.id = PANEL_SVG_VIEWER;
        svgInfo.name = "svg_viewer";
        svgInfo.title = "SVG Viewer";
        svgInfo.panel = new SVGViewer(m_notebook);
        svgInfo.defaultVisible = false;
        m_panels.push_back(svgInfo);
        // Don't add to notebook initially since it's hidden by default
        
    } catch (const std::exception& e) {
        // If panel creation fails, create a single error panel
        wxPanel* errorPanel = new wxPanel(m_notebook, wxID_ANY);
        wxStaticText* errorText = new wxStaticText(errorPanel, wxID_ANY, 
            wxString::Format("Panel creation error: %s\n\nThe application will still run with limited functionality.", e.what()));
        wxBoxSizer* errorSizer = new wxBoxSizer(wxVERTICAL);
        errorSizer->Add(errorText, 1, wxALL | wxCENTER, 20);
        errorPanel->SetSizer(errorSizer);
        
        m_notebook->AddPage(errorPanel, "Error");
        m_panels.clear();
    }
    
    // Set notebook as the main content
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(m_notebook, 1, wxEXPAND);
    SetSizer(mainSizer);
    
    Layout();
}

PanelInfo* MainFrame::FindPanelInfo(PanelID id)
{
    for (auto& panel : m_panels) {
        if (panel.id == id) {
            return &panel;
        }
    }
    return nullptr;
}

PanelInfo* MainFrame::FindPanelInfo(wxPanel* panel)
{
    for (auto& panelInfo : m_panels) {
        if (panelInfo.panel == panel) {
            return &panelInfo;
        }
    }
    return nullptr;
}

void MainFrame::UpdateMenuItems()
{
    // Update menu item check states based on panel visibility
    wxMenuBar* menuBar = GetMenuBar();
    if (menuBar) {
        // For notebook-based approach, panels are always "visible" if they exist
        // We could make this more sophisticated later
        menuBar->Check(ID_WINDOW_DRO, IsPanelVisible(PANEL_DRO));
        menuBar->Check(ID_WINDOW_JOG, IsPanelVisible(PANEL_JOG));
        menuBar->Check(ID_WINDOW_MACHINE_MANAGER, IsPanelVisible(PANEL_MACHINE_MANAGER));
        menuBar->Check(ID_WINDOW_GCODE_EDITOR, IsPanelVisible(PANEL_GCODE_EDITOR));
        menuBar->Check(ID_WINDOW_CONSOLE, IsPanelVisible(PANEL_CONSOLE));
        menuBar->Check(ID_WINDOW_MACRO, IsPanelVisible(PANEL_MACRO));
        menuBar->Check(ID_WINDOW_SVG_VIEWER, IsPanelVisible(PANEL_SVG_VIEWER));
        menuBar->Check(ID_WINDOW_SETTINGS, false); // Settings is special - not part of main UI
    }
}

// Dummy implementations for compatibility
void MainFrame::CreateDefaultLayout() {
    // Already handled in CreatePanels
}

void MainFrame::AddPanelToAui(PanelInfo& panelInfo) {
    // Not used in notebook version
}
