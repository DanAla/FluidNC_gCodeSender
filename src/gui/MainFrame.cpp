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
#include "UIQueue.h"
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

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, FluidNC::Version::GetFullVersionString(), wxDefaultPosition, wxSize(1200, 800))
    , m_mainToolbar(nullptr)
    , m_machineToolbar(nullptr) 
    , m_fileToolbar(nullptr)
{
    m_uiQueueTimer.SetOwner(this);
    m_uiQueueTimer.Start(10);
    LOG_INFO("MainFrame constructor - Begin initialization");
    
    // Initialize AUI manager first - this is required for everything else
    m_auiManager.SetManagedWindow(this);
    LOG_INFO("MainFrame - AUI manager initialized");
    
    // Initialize basic UI components first
    CreateMenuBar();
    CreateToolBars();
    CreateStatusBar();
    LOG_INFO("MainFrame - Basic UI components created");
    
    // Create and setup panels
    CreatePanels();
    SetupAuiManager();
    LOG_INFO("MainFrame - Panels created and AUI setup complete");
    
    // Initialize notification system
    NotificationSystem::Instance().SetParentWindow(this);
    LOG_INFO("MainFrame - Notification system initialized");
    
    // Delay all post-initialization tasks until the window is fully constructed
    CallAfter([this]() {
        LOG_INFO("MainFrame - Beginning post-initialization tasks");
        
        // 1. Initialize communication system
        SetupCommunicationCallbacks();
        LOG_INFO("MainFrame - Communication callbacks initialized");
        
        // 2. Update status and layout
        UpdateStatusBar();
        RestoreConnectionFirstLayout();
        UpdateMenuItems();
        m_auiManager.Update();
        LOG_INFO("MainFrame - Status and layout updated");
        
        // 3. Show welcome notifications
        NOTIFY_INFO("Connect to Machine", "Please connect to a CNC machine to begin using FluidNC gCode Sender.");
        
        // 4. Delay auto-connect attempt
        wxMilliSleep(100); // Small delay to ensure UI is responsive
        CallAfter([this]() {
            LOG_INFO("MainFrame - Attempting auto-connect if configured");
            PanelInfo* machineManagerInfo = FindPanelInfo(PANEL_MACHINE_MANAGER);
            if (machineManagerInfo) {
                MachineManagerPanel* machineManager = dynamic_cast<MachineManagerPanel*>(machineManagerInfo->panel);
                if (machineManager) {
                    LOG_INFO("MainFrame - Triggering auto-connect check");
                    machineManager->AttemptAutoConnect();
                }
            }
            
            // Show connection requirements after auto-connect attempt
            NOTIFY_WARNING("Connection Required", "Most features are disabled until you connect to a machine. Use Machine Manager to connect.");
        });
        
        // 5. Final window setup
        RestoreWindowGeometry();
        LOG_INFO("MainFrame - Post-initialization complete");
    });
}

MainFrame::~MainFrame()
{
    // Only destroy panels when the app is actually closing
    bool isAppClosing = wxTheApp && !wxTheApp->IsActive();
    
    // Hide all panels gracefully first
    for (auto& panelInfo : m_panels) {
        wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
        if (pane.IsOk()) {
            pane.Hide();
            LOG_INFO(wxString::Format("Hidden panel '%s'", panelInfo.name).ToStdString());
        }
    }
    
    // Commit the hide operations
    m_auiManager.Update();
    
    // Uninitialize AUI manager (this just removes the decorations, doesn't destroy panels)
    m_auiManager.UnInit();
    LOG_INFO("AUI manager uninitialized");
    
    // Only destroy panels if the app is actually closing
    if (isAppClosing) {
        LOG_INFO("Application is closing - cleaning up panels");
        for (auto& panelInfo : m_panels) {
            if (panelInfo.panel) {
                LOG_INFO(wxString::Format("Deleting panel '%s'", panelInfo.name).ToStdString());
                delete panelInfo.panel;
                panelInfo.panel = nullptr;
            }
        }
        m_panels.clear();
    } else {
        LOG_INFO("Frame closing but app still active - preserving panels");
    }
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

// Helper methods implementation
bool MainFrame::ShouldAllowPanelAccess(PanelID panelId) const {
    // Always allow access to essential panels
    if (panelId == PANEL_MACHINE_MANAGER || panelId == PANEL_CONSOLE) {
        return true;
    }
    
    // For other panels, require machine connection
    return HasMachineConnected();
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
void MainFrame::HandleConnectionStatusChange(const std::string& machineId, bool connected, bool isAutoConnect) {
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
            console->LogMessage("Connected to: " + machineName + " (ID: \"" + machineId + "\")", "INFO");
            console->LogMessage("Status: READY - Machine is active and awaiting commands", "INFO");
            console->LogMessage("=== END CONNECTION INFO ===", "INFO");
        } else {
            console->SetConnectionEnabled(false);
            
            // Log disconnection
            console->LogMessage("=== MACHINE DISCONNECTED ===", "WARNING");
            console->LogMessage("Machine: " + machineName + " (ID: \"" + machineId + "\")", "INFO");
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
    HandleConnectionStatusChange(machineId, connected, false);
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
    //     console->LogMessage("[\"" + machineId + "\"] " + message, level);
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
                HandleConnectionStatusChange(machineId, connected, false);
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


// Include the separated implementation files
// This is a simple, if unconventional, way to split a large file without
// modifying the build system. All code is effectively concatenated by the
// preprocessor into a single compilation unit.

#include "MainFrame_Events.cpp"
#include "MainFrame_Layouts.cpp"

void MainFrame::OnProcessUIQueue(wxTimerEvent& event)
{
    std::function<void()> func;
    while (UIQueue::getInstance().pop(func))
    {
        try
        {
            func();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Exception caught while processing UI queue: " + std::string(e.what()));
        }
    }
}

