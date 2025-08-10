/**
 * gui/SettingsDialog.h
 * Comprehensive settings dialog with tabbed interface
 * Tabs: General Settings, Job Settings, Machine Settings, Connection Settings
 */

#pragma once

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
// #include <wx/propgrid/propgrid.h>
// #include <wx/propgrid/advprops.h>
#include <wx/spinctrl.h>
#include <wx/filepicker.h>
#include <memory>
#include <vector>

class StateManager;
class ConnectionManager;
struct MachineConfig;
struct JobSettings;

/**
 * Multi-tab settings dialog for comprehensive application configuration
 * Features:
 * - General application preferences
 * - Current job settings (feeds, speeds, material)
 * - Machine-specific configurations
 * - Connection management (Telnet/USB/UART)
 */
class SettingsDialog : public wxDialog
{
public:
    SettingsDialog(wxWindow* parent, StateManager& stateManager, ConnectionManager& connectionManager);
    ~SettingsDialog();
    
    // Show specific tab
    void ShowTab(int tabIndex);
    void ShowGeneralTab() { ShowTab(0); }
    void ShowJobTab() { ShowTab(1); }
    void ShowMachineTab() { ShowTab(2); }
    void ShowConnectionTab() { ShowTab(3); }
    
private:
    // Event handlers
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnApply(wxCommandEvent& event);
    void OnResetToDefaults(wxCommandEvent& event);
    void OnTabChanged(wxBookCtrlEvent& event);
    
    // Tab creation methods
    void CreateGeneralTab(wxNotebook* notebook);
    void CreateJobTab(wxNotebook* notebook);
    void CreateMachineTab(wxNotebook* notebook);
    void CreateConnectionTab(wxNotebook* notebook);
    
    // Data loading/saving
    void LoadSettings();
    void SaveSettings();
    void LoadGeneralSettings();
    void SaveGeneralSettings();
    void LoadJobSettings();
    void SaveJobSettings();
    void LoadMachineSettings();
    void SaveMachineSettings();
    void LoadConnectionSettings();
    void SaveConnectionSettings();
    
    // Validation
    bool ValidateSettings();
    bool ValidateGeneralSettings();
    bool ValidateJobSettings();
    bool ValidateMachineSettings();
    bool ValidateConnectionSettings();
    
    // Machine management in settings
    void OnAddMachine(wxCommandEvent& event);
    void OnEditMachine(wxCommandEvent& event);
    void OnRemoveMachine(wxCommandEvent& event);
    void OnMachineSelectionChanged(wxListEvent& event);
    void UpdateMachineList();
    
    // Job profile management
    void OnSaveJobProfile(wxCommandEvent& event);
    void OnLoadJobProfile(wxCommandEvent& event);
    void OnDeleteJobProfile(wxCommandEvent& event);
    void UpdateJobProfileList();
    
    // Core references
    StateManager& m_stateManager;
    ConnectionManager& m_connectionManager;
    
    // Main notebook
    wxNotebook* m_notebook;
    
    // Tab panels
    wxPanel* m_generalPanel;
    wxPanel* m_jobPanel;
    wxPanel* m_machinePanel;
    wxPanel* m_connectionPanel;
    
    // General Settings Controls
    wxCheckBox* m_autoSaveCheck;
    wxSpinCtrl* m_autoSaveInterval;
    wxCheckBox* m_restoreLayoutCheck;
    wxCheckBox* m_showTooltipsCheck;
    wxChoice* m_unitsChoice;  // mm or inches
    wxChoice* m_coordinateSystemChoice;  // Absolute or Relative default
    wxDirPickerCtrl* m_workingDirectoryPicker;
    
    // Job Settings Controls
    wxTextCtrl* m_jobNameText;
    wxSpinCtrlDouble* m_feedRateSpinner;
    wxSpinCtrlDouble* m_spindleSpeedSpinner;
    wxSpinCtrlDouble* m_safeZSpinner;
    wxSpinCtrlDouble* m_workZSpinner;
    wxSpinCtrlDouble* m_depthPerPassSpinner;
    wxChoice* m_materialChoice;
    wxChoice* m_toolTypeChoice;
    wxSpinCtrlDouble* m_toolDiameterSpinner;
    wxListBox* m_jobProfilesList;
    
    // Machine Settings Controls
    wxListCtrl* m_machineList;
    // wxPropertyGrid* m_machinePropertyGrid;
    int m_selectedMachineIndex;
    
    // Connection Settings Controls
    wxCheckBox* m_autoConnectCheck;
    wxSpinCtrl* m_connectionTimeoutSpinner;
    wxSpinCtrl* m_maxRetriesSpinner;
    wxSpinCtrl* m_retryIntervalSpinner;
    wxCheckBox* m_keepAliveCheck;
    wxSpinCtrl* m_keepAliveIntervalSpinner;
    
    // Button controls
    wxButton* m_okButton;
    wxButton* m_cancelButton;
    wxButton* m_applyButton;
    wxButton* m_resetButton;
    
    // Material and tool type predefined lists
    std::vector<wxString> m_materials;
    std::vector<wxString> m_toolTypes;
    
    wxDECLARE_EVENT_TABLE();
};
