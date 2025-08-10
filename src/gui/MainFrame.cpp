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
#include <wx/artprov.h>
#include "NotificationSystem.h"

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
    , m_mainToolbar(nullptr)
    , m_machineToolbar(nullptr) 
    , m_fileToolbar(nullptr)
{
    // Initialize AUI manager
    m_auiManager.SetManagedWindow(this);
    
    // Create menu bar
    CreateMenuBar();
    
    // Create toolbars
    CreateToolBars();
    
    // Create status bar
    CreateStatusBar();
    
    // Create panels and setup AUI layout
    CreatePanels();
    SetupConnectionFirstLayout(); // Use connection-first layout instead of default
    
    // Update menu states
    UpdateMenuItems();
    
    // Commit all AUI changes
    m_auiManager.Update();
    
    // Initialize notification system
    NotificationSystem::Instance().SetParentWindow(this);
    
    // Show connection-focused welcome message
    NOTIFY_INFO("Connect to Machine", "Please connect to a CNC machine to begin using FluidNC gCode Sender.");
    
    // Explain the connection-first approach
    CallAfter([this]() {
        NOTIFY_WARNING("Connection Required", "Most features are disabled until you connect to a machine. Use Machine Manager to connect.");
    });
}

MainFrame::~MainFrame()
{
    // Properly uninitialize AUI manager
    m_auiManager.UnInit();
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

// AUI-based panel management implementations
void MainFrame::ShowPanel(PanelID panelId, bool show) {
    PanelInfo* panelInfo = FindPanelInfo(panelId);
    if (panelInfo) {
        wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo->name);
        if (pane.IsOk()) {
            if (show && !pane.IsShown()) {
                pane.Show(true);
                m_auiManager.Update();
            } else if (!show && pane.IsShown()) {
                pane.Show(false);
                m_auiManager.Update();
            }
        } else if (show) {
            // Panel not in AUI manager, add it
            AddPanelToAui(*panelInfo);
            m_auiManager.Update();
        }
        UpdateMenuItems();
    }
}

void MainFrame::TogglePanelVisibility(PanelID panelId) {
    bool isVisible = IsPanelVisible(panelId);
    ShowPanel(panelId, !isVisible);
}

bool MainFrame::IsPanelVisible(PanelID panelId) const {
    PanelInfo* panelInfo = const_cast<MainFrame*>(this)->FindPanelInfo(panelId);
    if (panelInfo) {
        wxAuiPaneInfo& pane = const_cast<MainFrame*>(this)->m_auiManager.GetPane(panelInfo->name);
        if (pane.IsOk()) {
            return pane.IsShown();
        }
    }
    return false;
}

void MainFrame::ResetLayout() {
    // Reset all panels to their default positions
    // Properly detach all existing panes first
    for (auto& panelInfo : m_panels) {
        wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
        if (pane.IsOk()) {
            m_auiManager.DetachPane(panelInfo.panel);
        }
    }
    
    // Re-add all default visible panels
    for (auto& panelInfo : m_panels) {
        if (panelInfo.defaultVisible) {
            AddPanelToAui(panelInfo);
        }
    }
    
    m_auiManager.Update();
    UpdateMenuItems();
    
    // Notify user that layout was reset
    NOTIFY_SUCCESS("Layout Reset", "All panels have been restored to their default positions.");
}

void MainFrame::SaveCurrentLayout() {
    // TODO: Save AUI perspective to config
    // wxString perspective = m_auiManager.SavePerspective();
    // StateManager::SavePerspective(perspective);
}

void MainFrame::LoadSavedLayout() {
    // TODO: Load AUI perspective from config
    // wxString perspective = StateManager::LoadPerspective();
    // if (!perspective.IsEmpty()) {
    //     m_auiManager.LoadPerspective(perspective, true);
    // }
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
    TogglePanelVisibility(PANEL_DRO);
}

void MainFrame::OnWindowJog(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle Jogging Panel");
    TogglePanelVisibility(PANEL_JOG);
}

void MainFrame::OnWindowSettings(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Settings Panel (modal dialog)");
    // Settings dialog currently disabled due to ConnectionManager dependency
    NOTIFY_INFO("Settings Unavailable", 
        "Settings dialog requires ConnectionManager which is currently disabled. "
        "Will be re-enabled in a future build when ConnectionManager is activated.");
}

void MainFrame::OnWindowMachineManager(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle Machine Manager Panel");
    TogglePanelVisibility(PANEL_MACHINE_MANAGER);
}

void MainFrame::OnWindowSVGViewer(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle SVG Viewer Panel");
    TogglePanelVisibility(PANEL_SVG_VIEWER);
}

void MainFrame::OnWindowGCodeEditor(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle G-code Editor Panel");
    TogglePanelVisibility(PANEL_GCODE_EDITOR);
}

void MainFrame::OnWindowMacro(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle Macro Panel");
    TogglePanelVisibility(PANEL_MACRO);
}

void MainFrame::OnWindowConsole(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Toggle Console Panel");
    TogglePanelVisibility(PANEL_CONSOLE);
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

// AUI-based panel creation
void MainFrame::CreatePanels()
{
    // Clear any existing panels
    m_panels.clear();
    
    try {
        // Create panels as direct children of the main frame
        
        // G-Code Editor - center panel
        PanelInfo gcodeInfo;
        gcodeInfo.id = PANEL_GCODE_EDITOR;
        gcodeInfo.name = "gcode_editor";
        gcodeInfo.title = "G-code Editor";
        gcodeInfo.panel = new GCodeEditor(this);
        gcodeInfo.defaultVisible = true;
        gcodeInfo.defaultPosition = "center";
        gcodeInfo.defaultSize = wxSize(600, 400);
        m_panels.push_back(gcodeInfo);
        
        // DRO Panel - left docking area
        PanelInfo droInfo;
        droInfo.id = PANEL_DRO;
        droInfo.name = "dro";
        droInfo.title = "Digital Readout";
        droInfo.panel = new DROPanel(this, nullptr); // nullptr for ConnectionManager
        droInfo.defaultVisible = true;
        droInfo.defaultPosition = "left";
        droInfo.defaultSize = wxSize(250, 200);
        m_panels.push_back(droInfo);
        
        // Jog Panel - left docking area (below DRO)
        PanelInfo jogInfo;
        jogInfo.id = PANEL_JOG;
        jogInfo.name = "jog";
        jogInfo.title = "Jogging Controls";
        jogInfo.panel = new JogPanel(this, nullptr); // nullptr for ConnectionManager
        jogInfo.defaultVisible = true;
        jogInfo.defaultPosition = "left";
        jogInfo.defaultSize = wxSize(250, 300);
        m_panels.push_back(jogInfo);
        
        // Console Panel - bottom docking area
        PanelInfo consoleInfo;
        consoleInfo.id = PANEL_CONSOLE;
        consoleInfo.name = "console";
        consoleInfo.title = "Terminal Console";
        consoleInfo.panel = new ConsolePanel(this);
        consoleInfo.defaultVisible = true;
        consoleInfo.defaultPosition = "bottom";
        consoleInfo.defaultSize = wxSize(800, 150);
        m_panels.push_back(consoleInfo);
        
        // Machine Manager Panel - right docking area
        PanelInfo machineInfo;
        machineInfo.id = PANEL_MACHINE_MANAGER;
        machineInfo.name = "machine_manager";
        machineInfo.title = "Machine Manager";
        machineInfo.panel = new MachineManagerPanel(this);
        machineInfo.defaultVisible = true;
        machineInfo.defaultPosition = "right";
        machineInfo.defaultSize = wxSize(300, 400);
        m_panels.push_back(machineInfo);
        
        // Macro Panel - right docking area (below machine manager)
        PanelInfo macroInfo;
        macroInfo.id = PANEL_MACRO;
        macroInfo.name = "macro";
        macroInfo.title = "Macro Panel";
        macroInfo.panel = new MacroPanel(this);
        macroInfo.defaultVisible = true;
        macroInfo.defaultPosition = "right";
        macroInfo.defaultSize = wxSize(300, 200);
        m_panels.push_back(macroInfo);
        
        // SVG Viewer - initially hidden, will be center when shown
        PanelInfo svgInfo;
        svgInfo.id = PANEL_SVG_VIEWER;
        svgInfo.name = "svg_viewer";
        svgInfo.title = "SVG Viewer";
        svgInfo.panel = new SVGViewer(this);
        svgInfo.defaultVisible = false;
        svgInfo.defaultPosition = "center";
        svgInfo.defaultSize = wxSize(400, 400);
        m_panels.push_back(svgInfo);
        
    } catch (const std::exception& e) {
        // If panel creation fails, create a single error panel
        wxPanel* errorPanel = new wxPanel(this, wxID_ANY);
        wxStaticText* errorText = new wxStaticText(errorPanel, wxID_ANY, 
            wxString::Format("Panel creation error: %s\n\nThe application will still run with limited functionality.", e.what()));
        wxBoxSizer* errorSizer = new wxBoxSizer(wxVERTICAL);
        errorSizer->Add(errorText, 1, wxALL | wxCENTER, 20);
        errorPanel->SetSizer(errorSizer);
        
        // Create minimal panel info for error panel
        PanelInfo errorInfo;
        errorInfo.id = PANEL_GCODE_EDITOR; // Use existing ID
        errorInfo.name = "error";
        errorInfo.title = "Error";
        errorInfo.panel = errorPanel;
        errorInfo.defaultVisible = true;
        errorInfo.defaultPosition = "center";
        m_panels.clear();
        m_panels.push_back(errorInfo);
    }
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

// Create toolbars
void MainFrame::CreateToolBars()
{
    // Create simple bitmaps for toolbar icons
    wxBitmap newBitmap = wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR, wxSize(16, 16));
    wxBitmap openBitmap = wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR, wxSize(16, 16));
    wxBitmap saveBitmap = wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR, wxSize(16, 16));
    
    // Main toolbar with common actions
    m_mainToolbar = CreateToolBar(wxTB_HORIZONTAL | wxTB_TEXT | wxTB_FLAT, wxID_ANY, "Main Toolbar");
    m_mainToolbar->SetToolBitmapSize(wxSize(16, 16));
    
    // Add basic toolbar items with proper bitmaps
    m_mainToolbar->AddTool(wxID_NEW, "New", newBitmap, "New file");
    m_mainToolbar->AddTool(wxID_OPEN, "Open", openBitmap, "Open file");
    m_mainToolbar->AddTool(wxID_SAVE, "Save", saveBitmap, "Save file");
    m_mainToolbar->AddSeparator();
    
    m_mainToolbar->Realize();
}

// Create status bar
void MainFrame::CreateStatusBar()
{
    m_statusBar = wxFrame::CreateStatusBar(STATUS_COUNT);
    
    // Set status bar field widths
    int widths[STATUS_COUNT] = { -1, 150, 100, 200 }; // Main field auto-sizes
    m_statusBar->SetStatusWidths(STATUS_COUNT, widths);
    
    // Set initial status
    SetStatusText("Ready", STATUS_MAIN);
    SetStatusText("No machine", STATUS_MACHINE);
    SetStatusText("Disconnected", STATUS_CONNECTION);
    SetStatusText("Position: ---", STATUS_POSITION);
}

// Setup AUI manager and add panels
void MainFrame::SetupAuiManager()
{
    // Add all panels to AUI manager
    for (auto& panelInfo : m_panels) {
        if (panelInfo.defaultVisible) {
            AddPanelToAui(panelInfo);
        }
    }
}

// Create default AUI layout
void MainFrame::CreateDefaultLayout()
{
    // This will be called after all panels are added to AUI
    // The AUI manager will automatically arrange them based on
    // the positions specified in AddPanelToAui
}

// Setup Connection-First layout - only show essential panels for connection
void MainFrame::SetupConnectionFirstLayout()
{
    // Only show Machine Manager and Console on startup
    // Hide all other panels until machine is connected
    for (auto& panelInfo : m_panels) {
        if (panelInfo.id == PANEL_MACHINE_MANAGER || panelInfo.id == PANEL_CONSOLE) {
            // Show connection-essential panels
            AddPanelToAui(panelInfo);
        }
        // All other panels remain hidden until connection is established
    }
    
    // Customize the layout for connection workflow
    wxAuiPaneInfo& machinePane = m_auiManager.GetPane("machine_manager");
    if (machinePane.IsOk()) {
        machinePane.Center().Layer(0).Position(0).BestSize(600, 400);
    }
    
    wxAuiPaneInfo& consolePane = m_auiManager.GetPane("console");
    if (consolePane.IsOk()) {
        consolePane.Bottom().Layer(1).Position(0).BestSize(800, 200);
    }
}

// Add a panel to the AUI manager
void MainFrame::AddPanelToAui(PanelInfo& panelInfo)
{
    // Safety check: Don't add if pane already exists
    wxAuiPaneInfo& existingPane = m_auiManager.GetPane(panelInfo.name);
    if (existingPane.IsOk()) {
        // Pane already exists, just show it instead of adding
        existingPane.Show(true);
        m_auiManager.Update();
        return;
    }
    
    wxAuiPaneInfo paneInfo;
    paneInfo.Name(panelInfo.name)
           .Caption(panelInfo.title)
           .BestSize(panelInfo.defaultSize)
           .MinSize(wxSize(200, 150))
           .CloseButton(panelInfo.canClose)
           .MaximizeButton(true)
           .MinimizeButton(false)
           .PinButton(true)
           .Dock()
           .Resizable(true)
           .FloatingSize(panelInfo.defaultSize);
    
    // Set docking position
    if (panelInfo.defaultPosition == "left") {
        paneInfo.Left().Layer(1);
    } else if (panelInfo.defaultPosition == "right") {
        paneInfo.Right().Layer(1);
    } else if (panelInfo.defaultPosition == "top") {
        paneInfo.Top().Layer(1);
    } else if (panelInfo.defaultPosition == "bottom") {
        paneInfo.Bottom().Layer(1);
    } else { // center
        paneInfo.Center().Layer(0);
    }
    
    m_auiManager.AddPane(panelInfo.panel, paneInfo);
}
