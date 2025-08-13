/**
 * gui/MainFrame.cpp
 * Main application window implementation (simple notebook-based version)
 */

#include "MainFrame.h"
#include "WelcomeDialog.h"
#include "AboutDialog.h"
#include "ProjectInfoDialog.h"
#include "DROPanel.h"
#include "JogPanel.h"
#include "MachineManagerPanel.h"
#include "ConsolePanel.h"
#include "GCodeEditor.h"
#include "MacroPanel.h"
#include "SVGViewer.h"
#include "MachineVisualizationPanel.h"
#include "SettingsDialog.h"
#include "core/SimpleLogger.h"
#include "core/Version.h"
#include "core/BuildCounter.h"
#include "core/ErrorHandler.h"
#include "core/CommunicationManager.h"
#include "core/StateManager.h"
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
    ID_PROJECT_INFO,
    ID_TEST_ERROR_HANDLER,
    ID_TEST_NOTIFICATION_SYSTEM,
    // Window menu items
    ID_WINDOW_DRO,
    ID_WINDOW_JOG,
    ID_WINDOW_SETTINGS,
    ID_WINDOW_MACHINE_MANAGER,
    ID_WINDOW_SVG_VIEWER,
    ID_WINDOW_GCODE_EDITOR,
    ID_WINDOW_MACRO,
    ID_WINDOW_CONSOLE,
    ID_WINDOW_RESET_LAYOUT,
    // Toolbar items
    ID_TOOLBAR_CONNECT_LAYOUT,
    ID_TOOLBAR_GCODE_LAYOUT,
    // Layout save
    ID_WINDOW_SAVE_LAYOUT
};

// Event table
wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(wxID_EXIT, MainFrame::OnExit)
    EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
    EVT_MENU(ID_SHOW_WELCOME, MainFrame::OnShowWelcome)
    EVT_MENU(ID_PROJECT_INFO, MainFrame::OnProjectInfo)
    EVT_MENU(ID_TEST_ERROR_HANDLER, MainFrame::OnTestErrorHandler)
    EVT_MENU(ID_TEST_NOTIFICATION_SYSTEM, MainFrame::OnTestNotificationSystem)
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
    EVT_MENU(ID_WINDOW_SAVE_LAYOUT, MainFrame::OnWindowSaveLayout)
    // Toolbar events
    EVT_MENU(ID_TOOLBAR_CONNECT_LAYOUT, MainFrame::OnToolbarConnectLayout)
    EVT_MENU(ID_TOOLBAR_GCODE_LAYOUT, MainFrame::OnToolbarGCodeLayout)
    // AUI events
    EVT_AUI_PANE_CLOSE(MainFrame::OnPaneClose)
    EVT_AUI_PANE_ACTIVATED(MainFrame::OnPaneActivated)
    EVT_AUI_PANE_BUTTON(MainFrame::OnPaneButton)
    EVT_AUI_RENDER(MainFrame::OnAuiRender)
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
    
    // Add all panels to AUI manager (even if initially hidden)
    SetupAuiManager();
    
    // Initialize status bar with proper information after panels are created
    CallAfter([this]() {
        UpdateStatusBar();
    });
    RestoreConnectionFirstLayout(); // Use connection-first layout and preserve splitter positions
    
    // Update menu states
    UpdateMenuItems();
    
    // Commit all AUI changes
    m_auiManager.Update();
    
    // Initialize notification system
    NotificationSystem::Instance().SetParentWindow(this);
    
    // Set up CommunicationManager callbacks for real machine communication
    SetupCommunicationCallbacks();
    
    // Attempt auto-connect if configured
    CallAfter([this]() {
        PanelInfo* machineManagerInfo = FindPanelInfo(PANEL_MACHINE_MANAGER);
        if (machineManagerInfo) {
            MachineManagerPanel* machineManager = dynamic_cast<MachineManagerPanel*>(machineManagerInfo->panel);
            if (machineManager) {
                // Trigger auto-connect check
                machineManager->AttemptAutoConnect();
            }
        }
    });
    
    // Show connection-focused welcome message
    NOTIFY_INFO("Connect to Machine", "Please connect to a CNC machine to begin using FluidNC gCode Sender.");
    
    // Explain the connection-first approach
    CallAfter([this]() {
        NOTIFY_WARNING("Connection Required", "Most features are disabled until you connect to a machine. Use Machine Manager to connect.");
    });
    
    // Restore window geometry after everything is initialized
    CallAfter([this]() {
        RestoreWindowGeometry();
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
    windowMenu->Append(ID_WINDOW_SAVE_LAYOUT, "&Save Layout\tCtrl+S", "Save current layout with splitter positions");
    windowMenu->Append(ID_WINDOW_RESET_LAYOUT, "&Reset Layout\tCtrl+R", "Reset all panels to default layout");
    menuBar->Append(windowMenu, "&Window");
    
    // Help menu
    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(ID_SHOW_WELCOME, "Show &Welcome Dialog", "Show the welcome dialog again");
    helpMenu->AppendSeparator();
    helpMenu->Append(ID_PROJECT_INFO, "Project &Information...", "Edit project implementation status and ToDo list");
    helpMenu->AppendSeparator();
    helpMenu->Append(ID_TEST_ERROR_HANDLER, "&Test Error Handler", "Test the error handling system");
    helpMenu->Append(ID_TEST_NOTIFICATION_SYSTEM, "Test &Notification System", "Test the notification system");
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

void MainFrame::OnProjectInfo(wxCommandEvent& WXUNUSED(event))
{
    ProjectInfoDialog dialog(this);
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

// Helper methods implementation
bool MainFrame::ShouldAllowPanelAccess(PanelID panelId) const {
    // Always allow access to essential panels
    if (panelId == PANEL_MACHINE_MANAGER || panelId == PANEL_CONSOLE) {
        return true;
    }
    
    // For other panels, require machine connection
    return HasMachineConnected();
}

void MainFrame::MinimizeNonEssentialPanels() {
    // Hide all non-essential panels (they can be restored later)
    for (auto& panelInfo : m_panels) {
        if (panelInfo.id != PANEL_MACHINE_MANAGER && panelInfo.id != PANEL_CONSOLE) {
            wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
            if (pane.IsOk() && pane.IsShown()) {
                // Hide the panel (user can restore via menu or when machine connects)
                pane.Show(false);
                LOG_INFO("Hid non-essential panel: " + panelInfo.name.ToStdString());
            }
        }
    }
}

void MainFrame::OnTestNotificationSystem(wxCommandEvent& WXUNUSED(event))
{
    // First, let's test if the parent window is set
    wxWindow* parent = NotificationSystem::Instance().GetParentWindow();
    wxString debugMsg = wxString::Format("NotificationSystem Debug: Parent window: %p, This window: %p", parent, this);
    LOG_INFO(debugMsg.ToStdString());
    
    // For now, let's go back to the simple approach but make it repeatable
    while (true) {
        wxString choice[] = {
            "Test Info Notification",
            "Test Success Notification", 
            "Test Warning Notification",
            "Test Error Notification",
            "Test Multiple Notifications",
            "Test Long Message Notification",
            "Exit Tests"
        };
        
        wxSingleChoiceDialog dialog(this, "Choose notification type to test (or Exit to close):", "Notification System Test", 7, choice);
        
        if (dialog.ShowModal() != wxID_OK) {
            break; // User cancelled
        }
        
        int selection = dialog.GetSelection();
        
        if (selection == 6) { // Exit Tests
            break;
        }
        
        switch (selection) {
            case 0:
                LOG_INFO("Testing Info notification...");
                NOTIFY_INFO("Test Information", "This is a test information notification. It should appear in the top-right corner and auto-dismiss after 5 seconds.");
                break;
                
            case 1:
                LOG_INFO("Testing Success notification...");
                NOTIFY_SUCCESS("Test Success", "This is a test success notification. Perfect for confirming completed operations.");
                break;
                
            case 2:
                LOG_INFO("Testing Warning notification...");
                NOTIFY_WARNING("Test Warning", "This is a test warning notification. It stays visible a bit longer to ensure the user sees important warnings.");
                break;
                
            case 3:
                LOG_INFO("Testing Error notification...");
                NOTIFY_ERROR("Test Error", "This is a test error notification. Error notifications have the longest duration and use red colors to draw attention.");
                break;
                
            case 4:
                LOG_INFO("Testing Multiple notifications...");
                NOTIFY_INFO("First Notification", "This is the first notification in a sequence.");
                wxMilliSleep(200);
                NOTIFY_SUCCESS("Second Notification", "This is the second notification, stacked below the first.");
                wxMilliSleep(200);
                NOTIFY_WARNING("Third Notification", "This is the third notification in the stack.");
                break;
                
            case 5:
                LOG_INFO("Testing Long message notification...");
                NOTIFY_INFO("Long Message Test", 
                    "This is a test of a longer notification message to demonstrate how the notification system handles text wrapping and larger content areas. The notification should automatically resize to accommodate the longer text while maintaining proper formatting and readability. The close button should remain easily accessible.");
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

// Dynamically update status bar field widths based on content
void MainFrame::UpdateStatusBarFieldWidths()
{
    if (!m_statusBar) return;
    
    try {
        // Get the current text in each field
        wxString mainText = m_statusBar->GetStatusText(STATUS_MAIN);
        wxString machineText = m_statusBar->GetStatusText(STATUS_MACHINE); 
        wxString connectionText = m_statusBar->GetStatusText(STATUS_CONNECTION);
        wxString positionText = m_statusBar->GetStatusText(STATUS_POSITION);
        
        // Calculate required widths for each field using text extent
        wxClientDC dc(m_statusBar);
        dc.SetFont(m_statusBar->GetFont());
        
        // Add padding for each field (left margin, right margin, and some extra space)
        int padding = 20;
        
        // Calculate width needed for each field
        wxSize machineSize = dc.GetTextExtent(machineText);
        int machineWidth = machineSize.GetWidth() + padding;
        
        wxSize connectionSize = dc.GetTextExtent(connectionText);
        int connectionWidth = connectionSize.GetWidth() + padding;
        
        wxSize positionSize = dc.GetTextExtent(positionText);
        int positionWidth = positionSize.GetWidth() + padding;
        
        // Set minimum widths to prevent fields from becoming too small
        int minMachineWidth = 120;
        int minConnectionWidth = 100;  // Minimum for "Disconnected"
        int minPositionWidth = 180;    // Minimum for position coordinates
        
        // Apply minimums
        machineWidth = std::max(machineWidth, minMachineWidth);
        connectionWidth = std::max(connectionWidth, minConnectionWidth);
        positionWidth = std::max(positionWidth, minPositionWidth);
        
        // Set up the field widths array
        // Field 0 (STATUS_MAIN) uses -1 to auto-size (takes remaining space)
        // Other fields use calculated widths
        int widths[STATUS_COUNT] = { 
            -1,                 // STATUS_MAIN - auto-size to fill remaining space
            machineWidth,       // STATUS_MACHINE - dynamic width based on content
            connectionWidth,    // STATUS_CONNECTION - dynamic width based on content
            positionWidth       // STATUS_POSITION - dynamic width based on content
        };
        
        // Apply the new widths
        m_statusBar->SetStatusWidths(STATUS_COUNT, widths);
        
        LOG_INFO(wxString::Format("Status bar widths updated: Machine=%d, Connection=%d, Position=%d", 
                                machineWidth, connectionWidth, positionWidth).ToStdString());
                                
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to update status bar field widths: " + std::string(e.what()));
    }
}

// Update toolbar button states based on connection status
void MainFrame::UpdateToolbarStates()
{
    if (!m_mainToolbar) return;
    
    bool hasMachineConnected = HasMachineConnected();
    
    // G-Code layout button should be disabled when no machine is connected
    m_mainToolbar->EnableTool(ID_TOOLBAR_GCODE_LAYOUT, hasMachineConnected);
    
    // Update tooltip text based on connection state
    if (hasMachineConnected) {
        m_mainToolbar->SetToolShortHelp(ID_TOOLBAR_GCODE_LAYOUT, "Restore G-Code Layout (Editor + Machine Visualization)");
    } else {
        m_mainToolbar->SetToolShortHelp(ID_TOOLBAR_GCODE_LAYOUT, "G-Code Layout (Connect to a machine first)");
    }
    
    m_mainToolbar->Refresh();
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
    
    // Use connection-first layout as the default
    SetupConnectionFirstLayout();
    
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

// Panel access methods
ConsolePanel* MainFrame::GetConsolePanel() const {
    PanelInfo* panelInfo = const_cast<MainFrame*>(this)->FindPanelInfo(PANEL_CONSOLE);
    if (panelInfo) {
        return dynamic_cast<ConsolePanel*>(panelInfo->panel);
    }
    return nullptr;
}

void MainFrame::UpdateMachineStatus(const std::string& machineId, const std::string& status) {
    // Find machine name from machine manager
    std::string machineName = "Unknown";
    PanelInfo* machineManagerInfo = FindPanelInfo(PANEL_MACHINE_MANAGER);
    if (machineManagerInfo) {
        MachineManagerPanel* machineManager = dynamic_cast<MachineManagerPanel*>(machineManagerInfo->panel);
        if (machineManager) {
            const auto& machines = machineManager->GetMachines();
            for (const auto& machine : machines) {
                if (machine.id == machineId) {
                    machineName = machine.name;
                    break;
                }
            }
        }
    }
    
    SetStatusText(wxString::Format("%s: %s", machineName, status), STATUS_MACHINE);
    UpdateStatusBar();
}

// UNIVERSAL CONNECTION STATUS HANDLER - THE ONLY method for connection updates
void MainFrame::HandleConnectionStatusChange(const std::string& machineId, bool connected) {
    LOG_INFO("HandleConnectionStatusChange: machineId=" + machineId + ", connected=" + (connected ? "true" : "false"));
    
    // Get machine info from machine manager
    std::string machineName = "Unknown Machine";
    MachineManagerPanel* machineManager = nullptr;
    
    PanelInfo* machineManagerInfo = FindPanelInfo(PANEL_MACHINE_MANAGER);
    if (machineManagerInfo) {
        machineManager = dynamic_cast<MachineManagerPanel*>(machineManagerInfo->panel);
        if (machineManager) {
            const auto& machines = machineManager->GetMachines();
            for (const auto& machine : machines) {
                if (machine.id == machineId) {
                    machineName = machine.name;
                    break;
                }
            }
        }
    }
    
    // 1. UPDATE MACHINE MANAGER PANEL
    if (machineManager) {
        machineManager->UpdateConnectionStatus(machineId, connected);
    }
    
    // 2. UPDATE STATUS BAR
    if (connected) {
        SetStatusText(wxString::Format("Connected to %s", machineName), STATUS_CONNECTION);
        LOG_INFO("Status bar updated: Connected to " + machineName);
    } else {
        SetStatusText("Disconnected", STATUS_CONNECTION);
        LOG_INFO("Status bar updated: Disconnected");
    }
    
    // 3. UPDATE GLOBAL CONNECTION STATE
    SetMachineConnected(connected);
    
    // 4. UPDATE CONSOLE PANEL
    ConsolePanel* console = GetConsolePanel();
    if (console) {
        if (connected) {
            console->SetConnectionEnabled(true, machineName);
            console->SetActiveMachine(machineId, machineName);
            
            // Log connection establishment
            console->LogMessage("=== CONNECTION ESTABLISHED ===", "INFO");
            console->LogMessage("Connected to: " + machineName + " (ID: " + machineId + ")", "INFO");
            console->LogMessage("Status: READY - Machine is active and awaiting commands", "INFO");
            console->LogMessage("=== END CONNECTION INFO ===", "INFO");
        } else {
            console->SetConnectionEnabled(false);
            
            // Log disconnection
            console->LogMessage("=== MACHINE DISCONNECTED ===", "WARNING");
            console->LogMessage("Machine: " + machineName + " (ID: " + machineId + ")", "INFO");
            console->LogMessage("=== MACHINE OFFLINE ===", "WARNING");
            
            // Show notification for unexpected disconnections
            if (!machineName.empty() && machineName != "Unknown Machine") {
                NOTIFY_ERROR("Machine Connection Lost", 
                    wxString::Format("Connection to '%s' has been lost!", machineName));
            }
        }
    }
    
    // 5. UPDATE UI STATES
    UpdateMenuItems();
    UpdateStatusBar();
    UpdateStatusBarFieldWidths();
    
    LOG_INFO("HandleConnectionStatusChange completed for " + machineName);
}

// Legacy method - redirects to HandleConnectionStatusChange
void MainFrame::UpdateConnectionStatus(const std::string& machineId, bool connected) {
    HandleConnectionStatusChange(machineId, connected);
}

void MainFrame::UpdateDRO(const std::string& machineId, const std::vector<float>& mpos, const std::vector<float>& wpos) {
    if (!mpos.empty() && mpos.size() >= 3) {
        // Use work position if available, otherwise machine position
        if (!wpos.empty() && wpos.size() >= 3) {
            SetStatusText(wxString::Format("WPos X:%.3f Y:%.3f Z:%.3f", wpos[0], wpos[1], wpos[2]), STATUS_POSITION);
        } else {
            SetStatusText(wxString::Format("MPos X:%.3f Y:%.3f Z:%.3f", mpos[0], mpos[1], mpos[2]), STATUS_POSITION);
        }
    } else {
        SetStatusText("Position: ---", STATUS_POSITION);
    }
    
    UpdateStatusBar();
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

void MainFrame::OnWindowSaveLayout(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Window menu: Save Layout");
    SaveCurrentLayoutBasedOnContext();
    NOTIFY_SUCCESS("Layout Saved", "Current layout and splitter positions have been saved.");
}

// Toolbar event handlers
void MainFrame::OnToolbarConnectLayout(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Toolbar: Restore Connection Layout");
    
    // Check if any machine is currently connected
    bool anyMachineConnected = HasMachineConnected();
    
    if (!anyMachineConnected) {
        LOG_INFO("No machines connected - checking for autoconnect machine");
        
        // Find machine manager and attempt autoconnect if available
        PanelInfo* machineManagerInfo = FindPanelInfo(PANEL_MACHINE_MANAGER);
        if (machineManagerInfo) {
            MachineManagerPanel* machineManager = dynamic_cast<MachineManagerPanel*>(machineManagerInfo->panel);
            if (machineManager) {
                // Check if there's a machine configured for autoconnect
                const auto& machines = machineManager->GetMachines();
                bool hasAutoConnectMachine = false;
                std::string autoConnectMachineName = "";
                
                for (const auto& machine : machines) {
                    if (machine.autoConnect && !machine.connected) {
                        hasAutoConnectMachine = true;
                        autoConnectMachineName = machine.name;
                        break;
                    }
                }
                
                if (hasAutoConnectMachine) {
                    LOG_INFO("Found autoconnect machine: " + autoConnectMachineName + " - attempting connection");
                    
                    // Show notification that we're attempting autoconnect
                    NOTIFY_INFO("Auto-Connecting", 
                        wxString::Format("No machines connected. Attempting to connect to '%s'...", autoConnectMachineName));
                    
                    // Trigger autoconnect attempt
                    machineManager->AttemptAutoConnect();
                } else {
                    LOG_INFO("No autoconnect machine found - just restoring layout");
                    
                    // Show notification that user needs to connect manually
                    NOTIFY_WARNING("No Connection", 
                        "No machines connected and no autoconnect machine configured. "
                        "Use Machine Manager to connect to a machine.");
                }
            } else {
                LOG_ERROR("Machine Manager panel not available");
            }
        } else {
            LOG_ERROR("Machine Manager panel not found");
        }
    } else {
        LOG_INFO("Machine already connected - just restoring layout");
    }
    
    // Always restore the connection layout regardless of connection status
    RestoreConnectionFirstLayout();
}

void MainFrame::OnToolbarGCodeLayout(wxCommandEvent& WXUNUSED(event)) {
    LOG_INFO("Toolbar: Restore G-Code Layout");
    
    // Check if any machine is currently connected
    bool anyMachineConnected = HasMachineConnected();
    
    if (!anyMachineConnected) {
        LOG_INFO("No machines connected - showing connection required message");
        
        // Show notification that connection is required for G-Code features
        NOTIFY_WARNING("Connection Required", 
            "G-Code features require an active machine connection. "
            "Please connect to a machine first using the Machine Manager.");
        
        // Instead of showing G-Code layout, show the Connection layout to help user connect
        RestoreConnectionFirstLayout();
        return;
    }
    
    // Machine is connected, proceed with G-Code layout
    RestoreGCodeLayout();
}

// AUI event handlers
void MainFrame::OnPaneClose(wxAuiManagerEvent& event) {
    // Save layouts whenever a pane is closed to preserve current state
    // Determine which layout we're in by checking visible panels
    
    bool hasGCodeEditor = IsPanelVisible(PANEL_GCODE_EDITOR);
    bool hasMachineVis = IsPanelVisible(PANEL_MACHINE_VISUALIZATION);
    bool hasMachineManager = IsPanelVisible(PANEL_MACHINE_MANAGER);
    bool hasConsole = IsPanelVisible(PANEL_CONSOLE);
    
    // Save appropriate layout based on current configuration
    if (hasGCodeEditor && hasMachineVis) {
        // Likely in G-Code layout
        SaveGCodeLayout();
    } else if (hasMachineManager && hasConsole) {
        // Likely in Connection layout
        SaveConnectionFirstLayout();
    }
    
    event.Skip(); // Allow the close to proceed
}

void MainFrame::OnPaneActivated(wxAuiManagerEvent& event) {
    // DISABLED: Too aggressive - was overwriting manual adjustments
    // Save layout when panes are activated (user clicks on different panes)
    // This can happen when user moves/docks panels
    // SaveCurrentLayoutBasedOnContext();
    event.Skip();
}

void MainFrame::OnPaneButton(wxAuiManagerEvent& event) {
    // DISABLED: Too aggressive - was overwriting manual adjustments
    // Save layout when pane buttons are clicked (minimize, maximize, pin, close)
    // This captures manual splitter adjustments and panel repositioning
    // SaveCurrentLayoutBasedOnContext();
    event.Skip();
}

void MainFrame::OnAuiRender(wxAuiManagerEvent& event) {
    // DISABLED: Too aggressive - was overwriting manual adjustments immediately
    // This event fires after AUI layout changes are rendered
    // Perfect for capturing splitter position changes and panel repositioning
    // static bool inRender = false;
    // if (!inRender) {
    //     inRender = true;
    //     CallAfter([this]() {
    //         SaveCurrentLayoutBasedOnContext();
    //         inRender = false;
    //     });
    // }
    event.Skip();
}

void MainFrame::RestoreWindowGeometry()
{
    try {
        // Get the saved window layout from StateManager
        WindowLayout layout = StateManager::getInstance().getWindowLayout("MainFrame");
        
        // If we have a saved layout, restore it
        if (layout.windowId == "MainFrame" && layout.width > 0 && layout.height > 0) {
            // Validate the position is on screen
            wxRect displayRect = wxGetDisplaySize();
            
            // Ensure window fits on screen
            int x = std::max(0, std::min(layout.x, displayRect.GetWidth() - layout.width));
            int y = std::max(0, std::min(layout.y, displayRect.GetHeight() - layout.height));
            
            // Ensure minimum size
            int width = std::max(400, layout.width);
            int height = std::max(300, layout.height);
            
            // Set position and size
            SetPosition(wxPoint(x, y));
            SetSize(wxSize(width, height));
            
            // Handle maximized state
            if (layout.maximized) {
                Maximize(true);
            }
            
            LOG_INFO("Restored MainFrame geometry: " + std::to_string(x) + "," + std::to_string(y) + " " + std::to_string(width) + "x" + std::to_string(height));
            
            // If position was corrected, save the corrected values immediately
            if (x != layout.x || y != layout.y) {
                CallAfter([this]() {
                    SaveWindowGeometry(); // Save the corrected position
                });
            }
        } else {
            // No saved layout, use defaults
            SetSize(1200, 800);
            Center();
            LOG_INFO("Using default MainFrame geometry (no saved layout found)");
        }
    } catch (const std::exception& e) {
        // If restoration fails, fall back to defaults
        SetSize(1200, 800);
        Center();
        LOG_ERROR("Failed to restore MainFrame geometry: " + std::string(e.what()) + " - using defaults");
    }
}

void MainFrame::SaveWindowGeometry()
{
    try {
        WindowLayout layout;
        layout.windowId = "MainFrame";
        
        // Get current window state
        layout.maximized = IsMaximized();
        
        if (!layout.maximized) {
            // Only save position/size if not maximized
            wxPoint pos = GetPosition();
            wxSize size = GetSize();
            
            layout.x = pos.x;
            layout.y = pos.y;
            layout.width = size.GetWidth();
            layout.height = size.GetHeight();
        } else {
            // For maximized window, save the normal (restored) size
            wxRect normalRect = GetRect();
            layout.x = normalRect.x;
            layout.y = normalRect.y;
            layout.width = normalRect.width;
            layout.height = normalRect.height;
        }
        
        layout.visible = IsShown();
        layout.docked = false; // MainFrame is never docked
        layout.dockingSide = "center";
        
        // Save to StateManager
        StateManager::getInstance().saveWindowLayout(layout);
        
        LOG_INFO("Saved MainFrame geometry: " + std::to_string(layout.x) + "," + std::to_string(layout.y) + 
                " " + std::to_string(layout.width) + "x" + std::to_string(layout.height) + 
                (layout.maximized ? " (maximized)" : ""));
                
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save MainFrame geometry: " + std::string(e.what()));
    }
}

// AUI-based panel creation
void MainFrame::CreatePanels()
{
    // Clear any existing panels
    m_panels.clear();
    
    try {
        // Create panels as direct children of the main frame
        
        // G-Code Editor
        PanelInfo gcodeInfo;
        gcodeInfo.id = PANEL_GCODE_EDITOR;
        gcodeInfo.name = "gcode_editor";
        gcodeInfo.title = "G-code Editor";
        gcodeInfo.panel = new GCodeEditor(this);
        gcodeInfo.defaultVisible = false;  // No default visibility - state dependent
        gcodeInfo.defaultPosition = "";     // No default position - state dependent
        gcodeInfo.defaultSize = wxSize(600, 400);
        m_panels.push_back(gcodeInfo);
        
        // DRO Panel
        PanelInfo droInfo;
        droInfo.id = PANEL_DRO;
        droInfo.name = "dro";
        droInfo.title = "Digital Readout";
        droInfo.panel = new DROPanel(this, nullptr); // nullptr for ConnectionManager
        droInfo.defaultVisible = false;  // No default visibility - state dependent
        droInfo.defaultPosition = "";     // No default position - state dependent
        droInfo.defaultSize = wxSize(250, 200);
        m_panels.push_back(droInfo);
        
        // Jog Panel
        PanelInfo jogInfo;
        jogInfo.id = PANEL_JOG;
        jogInfo.name = "jog";
        jogInfo.title = "Jogging Controls";
        jogInfo.panel = new JogPanel(this, nullptr); // nullptr for ConnectionManager
        jogInfo.defaultVisible = false;  // No default visibility - state dependent
        jogInfo.defaultPosition = "";     // No default position - state dependent
        jogInfo.defaultSize = wxSize(250, 300);
        m_panels.push_back(jogInfo);
        
        // Console Panel
        PanelInfo consoleInfo;
        consoleInfo.id = PANEL_CONSOLE;
        consoleInfo.name = "console";
        consoleInfo.title = "Terminal Console";
        consoleInfo.panel = new ConsolePanel(this);
        consoleInfo.canClose = true;             // Allow closing
        consoleInfo.defaultVisible = false;      // No default visibility - state dependent
        consoleInfo.defaultPosition = "";        // No default position - state dependent
        consoleInfo.defaultSize = wxSize(800, 150);
        m_panels.push_back(consoleInfo);
        
        // Machine Manager Panel
        PanelInfo machineInfo;
        machineInfo.id = PANEL_MACHINE_MANAGER;
        machineInfo.name = "machine_manager";
        machineInfo.title = "Machine Manager";
        machineInfo.panel = new MachineManagerPanel(this);
        machineInfo.defaultVisible = false;  // No default visibility - state dependent
        machineInfo.defaultPosition = "";      // No default position - state dependent
        machineInfo.defaultSize = wxSize(300, 400);
        m_panels.push_back(machineInfo);
        
        // Macro Panel
        PanelInfo macroInfo;
        macroInfo.id = PANEL_MACRO;
        macroInfo.name = "macro";
        macroInfo.title = "Macro Panel";
        macroInfo.panel = new MacroPanel(this);
        macroInfo.defaultVisible = false;  // No default visibility - state dependent
        macroInfo.defaultPosition = "";      // No default position - state dependent
        macroInfo.defaultSize = wxSize(300, 200);
        m_panels.push_back(macroInfo);
        
        // SVG Viewer
        PanelInfo svgInfo;
        svgInfo.id = PANEL_SVG_VIEWER;
        svgInfo.name = "svg_viewer";
        svgInfo.title = "SVG Viewer";
        svgInfo.panel = new SVGViewer(this);
        svgInfo.defaultVisible = false;  // No default visibility - state dependent
        svgInfo.defaultPosition = "";      // No default position - state dependent
        svgInfo.defaultSize = wxSize(400, 400);
        m_panels.push_back(svgInfo);
        
        // Machine Visualization Panel
        PanelInfo machineVisInfo;
        machineVisInfo.id = PANEL_MACHINE_VISUALIZATION;
        machineVisInfo.name = "machine_visualization";
        machineVisInfo.title = "Machine Visualization";
        machineVisInfo.panel = new MachineVisualizationPanel(this);
        machineVisInfo.defaultVisible = false;  // No default visibility - state dependent
        machineVisInfo.defaultPosition = "";      // No default position - state dependent
        machineVisInfo.defaultSize = wxSize(500, 400);
        m_panels.push_back(machineVisInfo);
        
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
        bool hasConnection = HasMachineConnected();
        
        // Update check states based on panel visibility
        menuBar->Check(ID_WINDOW_DRO, IsPanelVisible(PANEL_DRO));
        menuBar->Check(ID_WINDOW_JOG, IsPanelVisible(PANEL_JOG));
        menuBar->Check(ID_WINDOW_MACHINE_MANAGER, IsPanelVisible(PANEL_MACHINE_MANAGER));
        menuBar->Check(ID_WINDOW_GCODE_EDITOR, IsPanelVisible(PANEL_GCODE_EDITOR));
        menuBar->Check(ID_WINDOW_CONSOLE, IsPanelVisible(PANEL_CONSOLE));
        menuBar->Check(ID_WINDOW_MACRO, IsPanelVisible(PANEL_MACRO));
        menuBar->Check(ID_WINDOW_SVG_VIEWER, IsPanelVisible(PANEL_SVG_VIEWER));
        menuBar->Check(ID_WINDOW_SETTINGS, false); // Settings is special - not part of main UI
        
        // Enable/disable menu items based on connection status
        // Machine Manager and Console are always enabled (needed for connection management)
        menuBar->Enable(ID_WINDOW_MACHINE_MANAGER, true);
        menuBar->Enable(ID_WINDOW_CONSOLE, true);
        
        // All other panels require a machine connection to be useful
        menuBar->Enable(ID_WINDOW_DRO, hasConnection);
        menuBar->Enable(ID_WINDOW_JOG, hasConnection);
        menuBar->Enable(ID_WINDOW_GCODE_EDITOR, hasConnection);
        menuBar->Enable(ID_WINDOW_SVG_VIEWER, hasConnection);
        menuBar->Enable(ID_WINDOW_MACRO, hasConnection);
        menuBar->Enable(ID_WINDOW_SETTINGS, hasConnection);
        
        LOG_INFO(wxString::Format("UpdateMenuItems: Connection=%s, panels %s", 
                                hasConnection ? "YES" : "NO", 
                                hasConnection ? "enabled" : "disabled").ToStdString());
    }
    
    // Also update toolbar states whenever menu items are updated
    UpdateToolbarStates();
}

// Create toolbars
void MainFrame::CreateToolBars()
{
    // Create simple bitmaps for toolbar icons
    wxBitmap newBitmap = wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR, wxSize(16, 16));
    wxBitmap openBitmap = wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR, wxSize(16, 16));
    wxBitmap saveBitmap = wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR, wxSize(16, 16));
    wxBitmap connectBitmap = wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR, wxSize(16, 16));
    wxBitmap gcodeBitmap = wxArtProvider::GetBitmap(wxART_EXECUTABLE_FILE, wxART_TOOLBAR, wxSize(16, 16));
    
    // Main toolbar with common actions
    m_mainToolbar = CreateToolBar(wxTB_HORIZONTAL | wxTB_TEXT | wxTB_FLAT, wxID_ANY, "Main Toolbar");
    m_mainToolbar->SetToolBitmapSize(wxSize(16, 16));
    
    // Add basic toolbar items with proper bitmaps
    m_mainToolbar->AddTool(wxID_NEW, "New", newBitmap, "New file");
    m_mainToolbar->AddTool(wxID_OPEN, "Open", openBitmap, "Open file");
    m_mainToolbar->AddTool(wxID_SAVE, "Save", saveBitmap, "Save file");
    m_mainToolbar->AddSeparator();
    
    // Add Connect layout button - always enabled
    m_mainToolbar->AddTool(ID_TOOLBAR_CONNECT_LAYOUT, "Connect", connectBitmap, "Restore Connection Layout (Machine Manager + Console)");
    
    // Add G-code layout button - disabled when no connection
    m_mainToolbar->AddTool(ID_TOOLBAR_GCODE_LAYOUT, "G-Code", gcodeBitmap, "Restore G-Code Layout (Editor + Machine Visualization)");
    
    m_mainToolbar->Realize();
    
    // Initial toolbar state update
    UpdateToolbarStates();
}

// Create status bar
void MainFrame::CreateStatusBar()
{
    m_statusBar = wxFrame::CreateStatusBar(STATUS_COUNT);
    
    // Set initial status text first
    SetStatusText("Ready", STATUS_MAIN);
    SetStatusText("No machine", STATUS_MACHINE);
    SetStatusText("Disconnected", STATUS_CONNECTION);
    SetStatusText("Position: ---", STATUS_POSITION);
    
    // Set status bar field widths - CONNECTION field will be dynamically sized
    UpdateStatusBarFieldWidths();
}

// Setup AUI manager and add panels
void MainFrame::SetupAuiManager()
{
    // Add ALL panels to AUI manager, even if they start hidden
    // This ensures they can be shown later by layout restoration
    for (auto& panelInfo : m_panels) {
        AddPanelToAui(panelInfo);
        
        // Hide panels that shouldn't be visible by default
        if (!panelInfo.defaultVisible) {
            wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
            if (pane.IsOk()) {
                pane.Show(false);
            }
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
        if (panelInfo.id == PANEL_MACHINE_MANAGER) {
            // Add Machine Manager panel on left side - 40% width
            wxAuiPaneInfo paneInfo;
            paneInfo.Name(panelInfo.name)
                   .Caption(panelInfo.title)
                   .BestSize(480, 600)
                   .MinSize(300, 400)
                   .CloseButton(panelInfo.canClose)
                   .MaximizeButton(true)
                   .MinimizeButton(false)
                   .PinButton(true)
                   .Dock()
                   .Resizable(true)
                   .Left()      // Dock to left side
                   .Layer(0)    // Base layer
                   .Row(0);     // First row
            m_auiManager.AddPane(panelInfo.panel, paneInfo);
        } else if (panelInfo.id == PANEL_CONSOLE) {
            // Add Console panel to fill center (remaining space after left Machine Manager)
            wxAuiPaneInfo paneInfo;
            paneInfo.Name(panelInfo.name)
                   .Caption(panelInfo.title)
                   .BestSize(720, 600)
                   .MinSize(400, 150)
                   .CloseButton(true)      // Allow closing
                   .MaximizeButton(true)   // Allow maximizing
                   .MinimizeButton(true)   // Allow minimizing
                   .PinButton(true)        // Allow pinning/unpinning (floating)
                   .Dock()
                   .Resizable(true)
                   .Floatable(true)        // Make it floatable
                   .Movable(true)          // Allow moving to other positions
                   .Center()               // Fill center area (remaining space)
                   .Layer(0)               // Same layer as machine manager
                   .Row(0);                // Same row as machine manager
            m_auiManager.AddPane(panelInfo.panel, paneInfo);
        }
        // All other panels remain hidden until connection is established
    }
    
    // Update the AUI manager to apply the layout
    m_auiManager.Update();
    
    // DO NOT save immediately - let user adjust first
    // SaveConnectionFirstLayout(); // Removed - only save after user adjusts
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
    
    // Set docking position (only if defaultPosition is set)
    if (!panelInfo.defaultPosition.empty()) {
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
    } else {
        // No default position - use center as fallback
        paneInfo.Center().Layer(0);
    }
    
    m_auiManager.AddPane(panelInfo.panel, paneInfo);
}

// Setup communication callbacks for real machine communication
void MainFrame::SetupCommunicationCallbacks()
{
    ConsolePanel* console = GetConsolePanel();
    if (!console) {
        LOG_ERROR("ConsolePanel not available for communication callbacks");
        return;
    }
    
    // Set up CommunicationManager callbacks to integrate GUI with real machine communication
    CommunicationManager& commMgr = CommunicationManager::Instance();
    
    // Message callback - logs general messages to console
    // Note: Message callback now handled by MachineManagerPanel with machine names
    // commMgr.SetMessageCallback([console](const std::string& machineId, const std::string& message, const std::string& level) {
    //     console->LogMessage("[" + machineId + "] " + message, level);
    // });
    
    // Command sent callback - logs sent commands to console
    commMgr.SetCommandSentCallback([console](const std::string& machineId, const std::string& command) {
        console->LogSentCommand(command);
    });
    
    // Response received callback - logs received responses to console
    commMgr.SetResponseReceivedCallback([console](const std::string& machineId, const std::string& response) {
        console->LogReceivedResponse(response);
    });
    
    // Connection status callback - updates GUI connection state
    // CRITICAL: This callback is called from CommunicationManager threads, so we must use CallAfter for GUI operations
    commMgr.SetConnectionStatusCallback([this, console](const std::string& machineId, bool connected) {
        // THREAD SAFETY: Use CallAfter to ensure GUI operations happen on main thread
        CallAfter([this, console, machineId, connected]() {
            try {
                // Use ONLY the universal connection handler - no more multiple different paths
                HandleConnectionStatusChange(machineId, connected);
            } catch (const std::exception& e) {
                // Catch any exceptions to prevent crashes
                LOG_ERROR("Exception in connection status callback: " + std::string(e.what()));
                // Still try to show a basic error notification
                try {
                    NOTIFY_ERROR("Connection Error", "A connection status error occurred. Check the console for details.");
                } catch (...) {
                    // If even the notification fails, just log it
                    LOG_ERROR("Failed to show connection error notification");
                }
            }
        });
    });
    
    // DRO update callback - updates position displays
    commMgr.SetDROUpdateCallback([this](const std::string& machineId, const std::vector<float>& mpos, const std::vector<float>& wpos) {
        // Update status bar position display
        UpdateDRO(machineId, mpos, wpos);
        
        // TODO: Update DRO panel when available
        // Find and update DRO panel if visible
    });
    
    LOG_INFO("Communication callbacks configured for real machine communication");
}

// Comprehensive status bar update method
void MainFrame::UpdateStatusBar()
{
    try {
        // Get current time for general status
        wxDateTime now = wxDateTime::Now();
        wxString timeStr = now.Format("%H:%M:%S");
        
        // STATUS_MAIN field - Application status and general information
        std::string mainStatus = "Ready";
        
        // Check if any machine is connected
        bool anyConnected = false;
        std::string connectedMachineName = "";
        int totalMachines = 0;
        int connectedCount = 0;
        
        // Get machine status from machine manager
        PanelInfo* machineManagerInfo = FindPanelInfo(PANEL_MACHINE_MANAGER);
        if (machineManagerInfo) {
            MachineManagerPanel* machineManager = dynamic_cast<MachineManagerPanel*>(machineManagerInfo->panel);
            if (machineManager) {
                const auto& machines = machineManager->GetMachines();
                totalMachines = machines.size();
                
                for (const auto& machine : machines) {
                    if (machine.connected) {
                        anyConnected = true;
                        connectedCount++;
                        if (connectedMachineName.empty()) {
                            connectedMachineName = machine.name;
                        }
                    }
                }
            }
        }
        
        // Update main status based on connection state
        if (anyConnected) {
            if (connectedCount == 1) {
                mainStatus = "Active - Connected to machine";
            } else {
                mainStatus = wxString::Format("Active - %d machines connected", connectedCount).ToStdString();
            }
        } else if (totalMachines > 0) {
            mainStatus = wxString::Format("Ready - %d machine%s configured, none connected", 
                                        totalMachines, totalMachines == 1 ? "" : "s");
        } else {
            mainStatus = "Ready - No machines configured. Use Machine Manager to add machines.";
        }
        
        SetStatusText(mainStatus + " (" + timeStr + ")", STATUS_MAIN);
        
        // STATUS_MACHINE field - Current active machine info (if not already set by UpdateMachineStatus)
        wxString currentMachineText = GetStatusBar()->GetStatusText(STATUS_MACHINE);
        if (currentMachineText == "No machine" || currentMachineText.IsEmpty()) {
            if (!connectedMachineName.empty()) {
                SetStatusText(connectedMachineName + ": Ready", STATUS_MACHINE);
            } else if (totalMachines > 0) {
                SetStatusText(wxString::Format("%d configured", totalMachines), STATUS_MACHINE);
            } else {
                SetStatusText("No machines", STATUS_MACHINE);
            }
        }
        
        // STATUS_CONNECTION field - Connection status summary (if not already set by UpdateConnectionStatus)
        wxString currentConnectionText = GetStatusBar()->GetStatusText(STATUS_CONNECTION);
        if (currentConnectionText == "Disconnected" && anyConnected) {
            if (connectedCount == 1) {
                SetStatusText("Connected", STATUS_CONNECTION);
            } else {
                SetStatusText(wxString::Format("%d Connected", connectedCount), STATUS_CONNECTION);
            }
        } else if (!anyConnected) {
            SetStatusText("Disconnected", STATUS_CONNECTION);
        }
        
        // STATUS_POSITION field - Position info (if not already set by UpdateDRO)
        wxString currentPositionText = GetStatusBar()->GetStatusText(STATUS_POSITION);
        if (currentPositionText == "Position: ---" && anyConnected) {
            // Try to get position from CommunicationManager for the active machine
            // This is a fallback - normally UpdateDRO handles this
            SetStatusText("Position: Updating...", STATUS_POSITION);
        }
        
    } catch (const std::exception& e) {
        // If status bar update fails, at least show the error in main status
        SetStatusText(wxString::Format("Status update error: %s", e.what()), STATUS_MAIN);
        LOG_ERROR("UpdateStatusBar failed: " + std::string(e.what()));
    }
}

// Save the Connection-First layout state
void MainFrame::SaveConnectionFirstLayout()
{
    try {
        // Save the AUI perspective string for the connection-first layout
        wxString perspective = m_auiManager.SavePerspective();
        
        // Save to StateManager with a special key for connection-first layout
        StateManager::getInstance().setValue("ConnectionFirstLayout", perspective.ToStdString());
        
        LOG_INFO("Saved Connection-First layout perspective");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save Connection-First layout: " + std::string(e.what()));
    }
}

// Restore the Connection-First layout state
void MainFrame::RestoreConnectionFirstLayout()
{
    LOG_INFO("RestoreConnectionFirstLayout: Starting layout restoration");
    
    try {
        // Try to load saved ConnectionFirstLayout perspective first
        std::string savedPerspective = StateManager::getInstance().getValue<std::string>("ConnectionFirstLayout", "");
        
        LOG_INFO("RestoreConnectionFirstLayout: Saved perspective length: " + std::to_string(savedPerspective.length()));
        
        if (!savedPerspective.empty()) {
            // Load the saved perspective (includes splitter positions)
            wxString perspective = wxString::FromUTF8(savedPerspective.c_str());
            LOG_INFO("RestoreConnectionFirstLayout: Attempting to load saved perspective");
            
            if (m_auiManager.LoadPerspective(perspective, true)) {
                LOG_INFO("RestoreConnectionFirstLayout: Successfully loaded saved perspective - PRESERVING splitter positions");
                m_auiManager.Update();
                UpdateMenuItems();
                
                // Show notification about layout restoration
                NOTIFY_SUCCESS("Connection Layout Restored", "Saved layout with preserved splitter positions restored.");
                LOG_INFO("RestoreConnectionFirstLayout: Layout restoration completed successfully");
                return; // CRITICAL: Exit here to preserve the loaded layout
            } else {
                LOG_INFO("RestoreConnectionFirstLayout: LoadPerspective failed, falling back to manual setup");
            }
        } else {
            LOG_INFO("RestoreConnectionFirstLayout: No saved perspective found, using manual setup");
        }
        
        // Manual fallback setup - ensure required panels exist in AUI manager
        LOG_INFO("RestoreConnectionFirstLayout: Setting up manual fallback layout");
        
        // Only add panels if they don't exist (first-time setup)
        PanelInfo* machineManagerInfo = FindPanelInfo(PANEL_MACHINE_MANAGER);
        if (machineManagerInfo) {
            wxAuiPaneInfo& pane = m_auiManager.GetPane(machineManagerInfo->name);
            if (!pane.IsOk()) {
                LOG_INFO("RestoreConnectionFirstLayout: Adding Machine Manager panel to AUI manager");
                // Panel not in AUI manager, add it with default settings (first-time setup only)
                AddPanelToAui(*machineManagerInfo);
            } else {
                LOG_INFO("RestoreConnectionFirstLayout: Machine Manager panel already in AUI manager");
            }
        } else {
            LOG_ERROR("RestoreConnectionFirstLayout: Machine Manager panel not found!");
        }
        
        PanelInfo* consoleInfo = FindPanelInfo(PANEL_CONSOLE);
        if (consoleInfo) {
            wxAuiPaneInfo& pane = m_auiManager.GetPane(consoleInfo->name);
            if (!pane.IsOk()) {
                LOG_INFO("RestoreConnectionFirstLayout: Adding Console panel to AUI manager");
                // Panel not in AUI manager, add it with default settings (first-time setup only)
                AddPanelToAui(*consoleInfo);
            } else {
                LOG_INFO("RestoreConnectionFirstLayout: Console panel already in AUI manager");
            }
        } else {
            LOG_ERROR("RestoreConnectionFirstLayout: Console panel not found!");
        }
        
        // If no saved perspective or loading failed, set up the layout manually
        // First minimize non-essential panels instead of hiding them
        MinimizeNonEssentialPanels();
        
        // Ensure Machine Manager and Console are visible, but preserve user adjustments
        for (auto& panelInfo : m_panels) {
            if (panelInfo.id == PANEL_MACHINE_MANAGER || panelInfo.id == PANEL_CONSOLE) {
                wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
                if (pane.IsOk()) {
                    // Only show the pane - DO NOT reset size/position (preserves user splitter adjustments)
                    pane.Show(true);
                } else {
                    // Panel not in AUI manager, add it with default settings (first-time setup only)
                    wxAuiPaneInfo paneInfo;
                    if (panelInfo.id == PANEL_MACHINE_MANAGER) {
                        paneInfo.Name(panelInfo.name)
                               .Caption(panelInfo.title)
                               .BestSize(480, 600)
                               .MinSize(300, 400)
                               .Left()
                               .Layer(0)
                               .Row(0);
                    } else {
                        paneInfo.Name(panelInfo.name)
                               .Caption(panelInfo.title)
                               .BestSize(720, 600)
                               .MinSize(400, 150)
                               .Center()
                               .Layer(0)
                               .Row(0);
                    }
                    paneInfo.CloseButton(panelInfo.canClose)
                           .MaximizeButton(true)
                           .MinimizeButton(true)
                           .PinButton(true)
                           .Dock()
                           .Resizable(true)
                           .Movable(true)
                           .Floatable(true);
                    m_auiManager.AddPane(panelInfo.panel, paneInfo);
                }
            }
        }
        
        m_auiManager.Update();
        UpdateMenuItems();
        
        // Save this layout for future use
        SaveConnectionFirstLayout();
        
        // Show notification about layout restoration
        NOTIFY_SUCCESS("Connection Layout Restored", "Essential panels (Machine Manager + Console) are now active. Other panels are minimized.");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to restore Connection-First layout: " + std::string(e.what()));
        
        // Fallback to creating a new connection-first layout
        SetupConnectionFirstLayout();
        m_auiManager.Update();
        UpdateMenuItems();
    }
}

// Setup G-Code layout - G-code Editor on left, Machine Visualization on right
void MainFrame::SetupGCodeLayout()
{
    // Only show G-code Editor and Machine Visualization panels
    // Hide all other panels for focused G-code editing and visualization
    for (auto& panelInfo : m_panels) {
        if (panelInfo.id == PANEL_GCODE_EDITOR) {
            // Add G-code Editor panel on left side - 60% width
            wxAuiPaneInfo paneInfo;
            paneInfo.Name(panelInfo.name)
                   .Caption(panelInfo.title)
                   .BestSize(720, 600)
                   .MinSize(400, 300)
                   .CloseButton(panelInfo.canClose)
                   .MaximizeButton(true)
                   .MinimizeButton(false)
                   .PinButton(true)
                   .Dock()
                   .Resizable(true)
                   .Left()      // Dock to left side
                   .Layer(0)    // Base layer
                   .Row(0);     // First row
            m_auiManager.AddPane(panelInfo.panel, paneInfo);
        } else if (panelInfo.id == PANEL_MACHINE_VISUALIZATION) {
            // Add Machine Visualization panel to fill center (remaining space after left G-code Editor)
            wxAuiPaneInfo paneInfo;
            paneInfo.Name(panelInfo.name)
                   .Caption(panelInfo.title)
                   .BestSize(480, 600)
                   .MinSize(300, 200)
                   .CloseButton(true)      // Allow closing
                   .MaximizeButton(true)   // Allow maximizing
                   .MinimizeButton(true)   // Allow minimizing
                   .PinButton(true)        // Allow pinning/unpinning (floating)
                   .Dock()
                   .Resizable(true)
                   .Floatable(true)        // Make it floatable
                   .Movable(true)          // Allow moving to other positions
                   .Center()               // Fill center area (remaining space)
                   .Layer(0)               // Same layer as G-code editor
                   .Row(0);                // Same row as G-code editor
            m_auiManager.AddPane(panelInfo.panel, paneInfo);
        }
        // All other panels remain hidden for focused G-code work
    }
    
    // Update the AUI manager to apply the layout
    m_auiManager.Update();
    
    // DO NOT save immediately - let user adjust first
    // SaveGCodeLayout(); // Removed - only save after user adjusts
}

// Save the G-Code layout state
void MainFrame::SaveGCodeLayout()
{
    try {
        // Save the AUI perspective string for the G-code layout
        wxString perspective = m_auiManager.SavePerspective();
        
        // Save to StateManager with a special key for G-code layout
        StateManager::getInstance().setValue("GCodeLayout", perspective.ToStdString());
        
        LOG_INFO("Saved G-Code layout perspective");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save G-Code layout: " + std::string(e.what()));
    }
}

// Restore the G-Code layout state
void MainFrame::RestoreGCodeLayout()
{
    LOG_INFO("RestoreGCodeLayout: Starting layout restoration");
    
    try {
        // Try to load saved GCodeLayout perspective first
        std::string savedPerspective = StateManager::getInstance().getValue<std::string>("GCodeLayout", "");
        
        LOG_INFO("RestoreGCodeLayout: Saved perspective length: " + std::to_string(savedPerspective.length()));
        
        if (!savedPerspective.empty()) {
            // Load the saved perspective (includes splitter positions)
            wxString perspective = wxString::FromUTF8(savedPerspective.c_str());
            LOG_INFO("RestoreGCodeLayout: Attempting to load saved perspective");
            
            if (m_auiManager.LoadPerspective(perspective, true)) {
                LOG_INFO("RestoreGCodeLayout: Successfully loaded saved perspective - PRESERVING splitter positions");
                m_auiManager.Update();
                UpdateMenuItems();
                
                // CRITICAL: Connect G-code Editor and Machine Visualization panels even when restoring saved layout
                ConnectGCodePanels();
                
                // Show notification about layout restoration
                NOTIFY_SUCCESS("G-Code Layout Restored", "Saved layout with preserved splitter positions restored.");
                LOG_INFO("RestoreGCodeLayout: Layout restoration completed successfully");
                return; // CRITICAL: Exit here to preserve the loaded layout
            } else {
                LOG_INFO("RestoreGCodeLayout: LoadPerspective failed, falling back to manual setup");
            }
        } else {
            LOG_INFO("RestoreGCodeLayout: No saved perspective found, using manual setup");
        }
        
        // If no saved perspective or loading failed, set up the G-Code layout from scratch
        // This ensures proper G-Code layout positioning instead of generic AddPanelToAui
        
        // First hide all other panels for focused G-code work
        for (auto& panelInfo : m_panels) {
            if (panelInfo.id != PANEL_GCODE_EDITOR && panelInfo.id != PANEL_MACHINE_VISUALIZATION) {
                wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
                if (pane.IsOk() && pane.IsShown()) {
                    pane.Show(false);
                    LOG_INFO("Hid non-G-code panel: " + panelInfo.name.ToStdString());
                }
            }
        }
        
        // Ensure G-code Editor and Machine Visualization are visible, but preserve user adjustments
        for (auto& panelInfo : m_panels) {
            if (panelInfo.id == PANEL_GCODE_EDITOR || panelInfo.id == PANEL_MACHINE_VISUALIZATION) {
                wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
                if (pane.IsOk()) {
                    // Only show the pane - DO NOT reset size/position (preserves user splitter adjustments)
                    pane.Show(true);
                } else {
                    // Panel not in AUI manager, add it with G-Code layout specific settings (first-time setup only)
                    wxAuiPaneInfo paneInfo;
                    if (panelInfo.id == PANEL_GCODE_EDITOR) {
                        paneInfo.Name(panelInfo.name)
                               .Caption(panelInfo.title)
                               .BestSize(720, 600)
                               .MinSize(400, 300)
                               .Left()      // G-Code layout: Editor on left
                               .Layer(0)
                               .Row(0);
                    } else {
                        paneInfo.Name(panelInfo.name)
                               .Caption(panelInfo.title)
                               .BestSize(480, 600)
                               .MinSize(300, 200)
                               .Center()    // G-Code layout: Visualization in center
                               .Layer(0)
                               .Row(0);
                    }
                    paneInfo.CloseButton(panelInfo.canClose)
                           .MaximizeButton(true)
                           .MinimizeButton(true)
                           .PinButton(true)
                           .Dock()
                           .Resizable(true)
                           .Movable(true)
                           .Floatable(true);
                    m_auiManager.AddPane(panelInfo.panel, paneInfo);
                }
            }
        }
        
        
        m_auiManager.Update();
        UpdateMenuItems();
        
        // Connect G-code Editor and Machine Visualization panels
        ConnectGCodePanels();
        
        // Save this layout for future use
        SaveGCodeLayout();
        
        // Show notification about layout restoration
        NOTIFY_SUCCESS("G-Code Layout Restored", "G-code editing panels (Editor + Machine Visualization) are now active. Other panels are minimized.");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to restore G-Code layout: " + std::string(e.what()));
        
        // Fallback to creating a new G-code layout
        SetupGCodeLayout();
        m_auiManager.Update();
        UpdateMenuItems();
    }
}

// Connect G-Code Editor and Machine Visualization panels for real-time updates
void MainFrame::ConnectGCodePanels() {
    try {
        // Find the G-Code Editor panel
        PanelInfo* gcodeEditorInfo = FindPanelInfo(PANEL_GCODE_EDITOR);
        if (!gcodeEditorInfo) {
            LOG_WARNING("ConnectGCodePanels: G-Code Editor panel not found");
            return;
        }
        
        GCodeEditor* gcodeEditor = dynamic_cast<GCodeEditor*>(gcodeEditorInfo->panel);
        if (!gcodeEditor) {
            LOG_ERROR("ConnectGCodePanels: G-Code Editor panel cast failed");
            return;
        }
        
        // Find the Machine Visualization panel
        PanelInfo* machineVisInfo = FindPanelInfo(PANEL_MACHINE_VISUALIZATION);
        if (!machineVisInfo) {
            LOG_WARNING("ConnectGCodePanels: Machine Visualization panel not found");
            return;
        }
        
        MachineVisualizationPanel* machineVis = dynamic_cast<MachineVisualizationPanel*>(machineVisInfo->panel);
        if (!machineVis) {
            LOG_ERROR("ConnectGCodePanels: Machine Visualization panel cast failed");
            return;
        }
        
        // Set up the callback from G-Code Editor to Machine Visualization
        gcodeEditor->SetTextChangeCallback([machineVis](const std::string& gcodeText) {
            try {
                LOG_INFO("MainFrame callback: Received G-code text of length: " + std::to_string(gcodeText.length()));
                // Update the visualization with the new G-code content
                machineVis->SetGCodeContent(wxString::FromUTF8(gcodeText));
                
                LOG_INFO("G-Code visualization updated from editor change");
                
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to update G-Code visualization: " + std::string(e.what()));
            }
        });
        
        // Also update visualization with current G-code content immediately
        std::string currentGCode_std = gcodeEditor->GetText();
        if (!currentGCode_std.empty()) {
            machineVis->SetGCodeContent(wxString::FromUTF8(currentGCode_std));
        }
        
        LOG_INFO("Successfully connected G-Code Editor and Machine Visualization panels");
        
        // Show notification about the connection
        NOTIFY_SUCCESS("G-Code Panels Connected", 
            "G-Code Editor is now linked to Machine Visualization. Changes will update in real-time.");
        
    } catch (const std::exception& e) {
        LOG_ERROR("ConnectGCodePanels failed: " + std::string(e.what()));
        
        // Show error notification to user
        NOTIFY_ERROR("G-Code Panel Connection Failed", 
            "Could not connect G-Code Editor to Machine Visualization. Real-time updates may not work.");
    }
}

// Smart layout saving based on current context
void MainFrame::SaveCurrentLayoutBasedOnContext() {
    try {
        // Determine which layout we're in by checking visible panels
        bool hasGCodeEditor = IsPanelVisible(PANEL_GCODE_EDITOR);
        bool hasMachineVis = IsPanelVisible(PANEL_MACHINE_VISUALIZATION);
        bool hasMachineManager = IsPanelVisible(PANEL_MACHINE_MANAGER);
        bool hasConsole = IsPanelVisible(PANEL_CONSOLE);
        
        // Save appropriate layout based on current configuration
        if (hasGCodeEditor && hasMachineVis) {
            // Likely in G-Code layout - save it
            SaveGCodeLayout();
        } else if (hasMachineManager && hasConsole) {
            // Likely in Connection layout - save it
            SaveConnectionFirstLayout();
        }
        // If we can't determine context, don't save to avoid corrupting layouts
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save current layout based on context: " + std::string(e.what()));
    }
}
