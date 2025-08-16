/**
 * gui/MainFrame_Events.cpp
 *
 * Event handling implementation for the MainFrame class.
 *
 * This file is part of the MainFrame refactoring to split its functionality
 * into smaller, more manageable parts.
 */

#include "MainFrame.h"
#include "WelcomeDialog.h"
#include "AboutDialog.h"
#include "ProjectInfoDialog.h"
#include "SettingsDialog.h"
#include "core/SimpleLogger.h"
#include "core/ErrorHandler.h"
#include "NotificationSystem.h"
#include "MachineManagerPanel.h"
#include <wx/msgdlg.h>

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

void MainFrame::OnClose(wxCloseEvent& event)
{
    try {
        // Save window state before hiding or destroying
        SaveWindowGeometry();
        SaveCurrentLayoutBasedOnContext();
        
        // If the app is quitting, proceed with destruction
        if (wxTheApp && !wxTheApp->IsActive()) {
            event.Skip();  // Allow default processing
        } else {
            // Just hide the frame if the app isn't quitting
            Hide();
        }
    } catch (const std::exception& e) {
        // Log error but continue with shutdown
        LOG_ERROR("Error during window close: " + std::string(e.what()));
        event.Skip();
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
    // Don't allow any panel to be destroyed, just hide them
    if (event.pane) {
        // Note: We keep the close button visible but just hide the panel
        event.pane->Hide();
        m_auiManager.Update();
        
        // Save layouts whenever a pane is hidden to preserve current state
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
        
        // Prevent the actual panel closure/destruction
        event.Veto();
        
        LOG_INFO(wxString::Format("Panel '%s' hidden but preserved", event.pane->name).ToStdString());
    }
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


