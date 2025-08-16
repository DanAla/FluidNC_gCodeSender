#pragma once

#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/statusbr.h>
#include <wx/timer.h>
#include <wx/toolbar.h>
#include <memory>
#include <map>
#include <vector>

class StateManager;
class ConsolePanel;
class MachineManagerPanel; // Forward declare to resolve circular dependency

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
    wxString defaultPosition = "center";
    wxSize defaultSize = wxSize(300, 200);
};

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
    ConsolePanel* GetConsolePanel() const;
    
    // Machine status updates
    void UpdateMachineStatus(const std::string& machineId, const std::string& status);
    void UpdateDRO(const std::string& machineId, const std::vector<float>& mpos, const std::vector<float>& wpos);
    
    // UNIVERSAL connection status handler
    void HandleConnectionStatusChange(const std::string& machineId, bool connected, bool isAutoConnect);
    
    // Legacy method - redirects to HandleConnectionStatusChange
    void UpdateConnectionStatus(const std::string& machineId, bool connected);
    
private:
    // Event handlers
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnShowWelcome(wxCommandEvent& event);
    void OnProjectInfo(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnTestErrorHandler(wxCommandEvent& event);
    void OnTestNotificationSystem(wxCommandEvent& event);
    
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
    
    // Toolbar handlers
    void OnToolbarConnectLayout(wxCommandEvent& event);
    void OnToolbarGCodeLayout(wxCommandEvent& event);
    
    // AUI event handlers
    void OnPaneClose(wxAuiManagerEvent& event);
    void OnPaneActivated(wxAuiManagerEvent& event);
    void OnPaneButton(wxAuiManagerEvent& event);
    void OnAuiRender(wxAuiManagerEvent& event);
    
    // UI Creation
    void CreateMenuBar();
    void CreateToolBars();
    void CreateStatusBar();
    void CreatePanels();
    void SetupAuiManager();
    
    // Layout management
    void CreateDefaultLayout();
    void SetupConnectionFirstLayout();
    void SaveConnectionFirstLayout();
    void RestoreConnectionFirstLayout();
    void SetupGCodeLayout();
    void SaveGCodeLayout();
    void RestoreGCodeLayout();
    void SaveCurrentLayoutBasedOnContext();
    void SaveWindowGeometry();
    void RestoreWindowGeometry();
    void UpdateMenuItems();
    void UpdateToolbarStates();
    void UpdateStatusBar();
    void UpdateStatusBarFieldWidths(); // Added missing declaration

    // Utility functions
    PanelInfo* FindPanelInfo(PanelID id);
    PanelInfo* FindPanelInfo(wxPanel* panel);
    wxAuiPaneInfo AddPanelToAui(PanelInfo& panelInfo);
    
    // Communication setup
    void SetupCommunicationCallbacks();
    
    // G-Code panel integration
    void ConnectGCodePanels();
    
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
    
    // Connection state tracking
    bool m_hasMachineConnected = false;
    
    // Helper methods
    bool HasMachineConnected() const { return m_hasMachineConnected; }
    void SetMachineConnected(bool connected) { m_hasMachineConnected = connected; }
    bool ShouldAllowPanelAccess(PanelID panelId) const;
    void MinimizeNonEssentialPanels();

    // UI Queue Timer
    wxTimer m_uiQueueTimer;
    void OnProcessUIQueue(wxTimerEvent& event);
    
    wxDECLARE_EVENT_TABLE();
};
