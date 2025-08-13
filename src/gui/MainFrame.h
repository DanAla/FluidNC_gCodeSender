/**
 * gui/MainFrame.h
 * Professional MDI main frame with flexible docking, toolbars, status bars
 * Supports multiple machines, dynamic panel management, and full layout persistence
 */

#pragma once

#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/statusbr.h>
#include <wx/toolbar.h>
#include <memory>
#include <map>
#include <vector>

class StateManager;
class ConnectionManager;
class DROPanel;
class JogPanel;
class SettingsDialog;
class MachineManagerPanel;
class SVGViewer;
class GCodeEditor;
class MacroPanel;
class ConsolePanel;

// Panel IDs for consistent identification
enum PanelID {
    PANEL_DRO = 1000,
    PANEL_JOG,
    PANEL_MACHINE_MANAGER,
    PANEL_SVG_VIEWER,
    PANEL_GCODE_EDITOR,
    PANEL_MACRO,
    PANEL_CONSOLE,
    PANEL_MACHINE_VISUALIZATION
};

// Panel information structure
struct PanelInfo {
    wxString name;
    wxString title;
    wxPanel* panel;
    PanelID id;
    bool canClose = true;
    bool defaultVisible = true;
    wxString defaultPosition = "center";  // left, right, top, bottom, center
    wxSize defaultSize = wxSize(300, 200);
};

/**
 * Professional CNC control application main frame
 * Features:
 * - Flexible docking with AUI
 * - Multiple toolbar support
 * - Multi-field status bar
 * - Menu-driven panel visibility
 * - Complete layout persistence
 * - Multi-machine support
 */
class MainFrame : public wxFrame
{
public:
    MainFrame();
    ~MainFrame();
    
    // Panel management
    void ShowPanel(PanelID panelId, bool show = true);
    void TogglePanelVisibility(PanelID panelId);
    bool IsPanelVisible(PanelID panelId) const;
    void ResetLayout();
    void SaveCurrentLayout();
    void LoadSavedLayout();
    
    // Panel access
    class ConsolePanel* GetConsolePanel() const;
    
    // Machine status updates
    void UpdateMachineStatus(const std::string& machineId, const std::string& status);
    void UpdateDRO(const std::string& machineId, const std::vector<float>& mpos, const std::vector<float>& wpos);
    
    // UNIVERSAL connection status handler - THE ONLY method for connection updates
    void HandleConnectionStatusChange(const std::string& machineId, bool connected);
    
    // Legacy method - kept for compatibility but redirects to HandleConnectionStatusChange
    void UpdateConnectionStatus(const std::string& machineId, bool connected);
    
private:
    // Event handlers
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnShowWelcome(wxCommandEvent& event);
    void OnProjectInfo(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    
    // Window menu event handlers
    void OnWindowDRO(wxCommandEvent& event);
    void OnWindowJog(wxCommandEvent& event);
    void OnWindowSettings(wxCommandEvent& event);
    void OnWindowMachineManager(wxCommandEvent& event);
    void OnWindowSVGViewer(wxCommandEvent& event);
    void OnWindowGCodeEditor(wxCommandEvent& event);
    void OnWindowMacro(wxCommandEvent& event);
    void OnWindowConsole(wxCommandEvent& event);
    void OnWindowResetLayout(wxCommandEvent& event);
    void OnWindowSaveLayout(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);
    
    // Menu handlers
    void OnNewMachine(wxCommandEvent& event);
    void OnEditMachines(wxCommandEvent& event);
    void OnConnectAll(wxCommandEvent& event);
    void OnDisconnectAll(wxCommandEvent& event);
    void OnEmergencyStop(wxCommandEvent& event);
    void OnSettings(wxCommandEvent& event);
    void OnTogglePanel(wxCommandEvent& event);
    void OnResetLayout(wxCommandEvent& event);
    void OnTestErrorHandler(wxCommandEvent& event);
    void OnTestNotificationSystem(wxCommandEvent& event);
    
    // Toolbar handlers
    void OnToolbarConnect(wxCommandEvent& event);
    void OnToolbarDisconnect(wxCommandEvent& event);
    void OnToolbarEmergencyStop(wxCommandEvent& event);
    void OnToolbarHome(wxCommandEvent& event);
    void OnToolbarJog(wxCommandEvent& event);
    void OnToolbarConnectLayout(wxCommandEvent& event);
    void OnToolbarGCodeLayout(wxCommandEvent& event);
    
    // AUI event handlers
    void OnPaneClose(wxAuiManagerEvent& event);
    void OnPaneActivated(wxAuiManagerEvent& event);
    void OnPaneButton(wxAuiManagerEvent& event);
    void OnAuiRender(wxAuiManagerEvent& event);
    
    // Layout handlers
    // TODO: Re-add AUI handlers when AUI is enabled
    
    // UI Creation
    void CreateMenuBar();
    void CreateToolBars();
    void CreateStatusBar();
    void CreatePanels();
    void SetupAuiManager();
    
    // Layout management
    void CreateDefaultLayout();
    void SetupConnectionFirstLayout(); // Connection-focused startup layout
    void SaveConnectionFirstLayout(); // Save the connection-first layout
    void RestoreConnectionFirstLayout(); // Restore connection-first layout
    void SetupGCodeLayout(); // G-code editing and visualization layout
    void SaveGCodeLayout(); // Save the G-code layout
    void RestoreGCodeLayout(); // Restore G-code layout
    void SaveCurrentLayoutBasedOnContext(); // Smart save based on visible panels
    void SaveWindowGeometry();
    void RestoreWindowGeometry();
    void UpdateMenuItems();
    void UpdateToolbarStates();
    void UpdateStatusBar();
    void UpdateStatusBarFieldWidths();
    
    // Utility functions
    PanelInfo* FindPanelInfo(PanelID id);
    PanelInfo* FindPanelInfo(wxPanel* panel);
    void AddPanelToAui(PanelInfo& panelInfo);
    
    // Communication setup
    void SetupCommunicationCallbacks();
    
    // G-Code panel integration
    void ConnectGCodePanels();
    
    // Core components - temporarily disabled
    // StateManager& m_stateManager;
    // std::unique_ptr<ConnectionManager> m_connectionManager;
    
    // AUI Manager for docking
    wxAuiManager m_auiManager;
    
    // Toolbars
    wxToolBar* m_mainToolbar;
    wxToolBar* m_machineToolbar;
    wxToolBar* m_fileToolbar;
    
    // Status bar
    wxStatusBar* m_statusBar;
    enum StatusField {
        STATUS_MAIN = 0,
        STATUS_MACHINE,
        STATUS_CONNECTION,
        STATUS_POSITION,
        STATUS_COUNT
    };
    
    // Panels storage
    std::vector<PanelInfo> m_panels;
    
    // Menu references for dynamic updates
    wxMenu* m_viewMenu;
    wxMenu* m_machineMenu;
    std::map<PanelID, wxMenuItem*> m_panelMenuItems;
    
    // Current machine context (simplified)
    // std::string m_currentMachine;
    
    // Connection state tracking
    bool m_hasMachineConnected = false;
    
    // Helper methods
    bool HasMachineConnected() const { return m_hasMachineConnected; }
    void SetMachineConnected(bool connected) { m_hasMachineConnected = connected; }
    bool ShouldAllowPanelAccess(PanelID panelId) const;
    void MinimizeNonEssentialPanels();
    
    wxDECLARE_EVENT_TABLE();
};
