/**
 * gui/EnhancedEditMachineDialog.h
 * Enhanced machine configuration dialog with kinematics-aware homing support
 * Multi-tab interface for comprehensive machine settings including auto-discovery
 */

#pragma once

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/statbox.h>
#include <wx/progdlg.h>
#include "../core/MachineConfigManager.h"
#include "../core/HomingManager.h"

/**
 * Enhanced Edit Machine Dialog with comprehensive machine configuration
 * Features:
 * - Basic machine settings (connection, name, etc.)
 * - Machine capability auto-discovery (Query Machine button)
 * - Kinematics-aware homing configuration
 * - Custom homing sequence editor
 * - Real-time homing sequence testing
 * - User preference settings
 */
class EnhancedEditMachineDialog : public wxDialog {
public:
    EnhancedEditMachineDialog(wxWindow* parent, const std::string& machineId = "", bool isNewMachine = true);
    ~EnhancedEditMachineDialog();
    
    // Get the configured machine data
    EnhancedMachineConfig GetMachineConfig() const;
    
    // Set machine data for editing
    void SetMachineConfig(const EnhancedMachineConfig& config);
    
private:
    // Event handlers
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnApply(wxCommandEvent& event);
    void OnQueryMachine(wxCommandEvent& event);
    void OnTestHoming(wxCommandEvent& event);
    void OnAutoDetectKinematics(wxCommandEvent& event);
    
    // Tab creation
    void CreateNotebook();
    void CreateBasicTab(wxNotebook* notebook);
    void CreateCapabilitiesTab(wxNotebook* notebook);
    void CreateHomingTab(wxNotebook* notebook);
    void CreateUserSettingsTab(wxNotebook* notebook);
    
    // Basic tab controls
    void CreateBasicControls(wxPanel* panel);
    void CreateConnectionSettings(wxPanel* panel);
    
    // Capabilities tab controls
    void CreateCapabilityControls(wxPanel* panel);
    void CreateDiscoveryControls(wxPanel* panel);
    
    // Homing tab controls
    void CreateHomingControls(wxPanel* panel);
    void CreateHomingSequenceControls(wxPanel* panel);
    void CreateCustomSequenceEditor(wxPanel* panel);
    
    // User settings tab controls
    void CreateUserControls(wxPanel* panel);
    
    // Event handlers for homing settings
    void OnHomingSequenceChanged(wxCommandEvent& event);
    void OnAddHomingStep(wxCommandEvent& event);
    void OnRemoveHomingStep(wxCommandEvent& event);
    void OnMoveHomingStepUp(wxCommandEvent& event);
    void OnMoveHomingStepDown(wxCommandEvent& event);
    void OnHomingStepSelected(wxListEvent& event);
    void OnHomingStepEdit(wxListEvent& event);
    
    // Machine capability discovery
    void QueryMachineCapabilities();
    void OnQueryProgress(const std::string& message);
    void OnQueryComplete(const MachineCapabilities& capabilities);
    void OnQueryError(const std::string& error);
    
    // Homing sequence testing
    void TestHomingSequence();
    void OnHomingProgress(const std::string& machineId, const HomingProgress& progress);
    void OnHomingComplete(const std::string& machineId);
    
    // Data validation and management
    bool ValidateInput();
    void LoadMachineData();
    void SaveMachineData();
    void UpdateUI();
    void UpdateHomingSequenceUI();
    void UpdateCustomSequenceUI();
    
    // Helper methods
    std::vector<std::string> GetCurrentHomingSequence();
    void SetHomingSequence(const std::vector<std::string>& sequence);
    void PopulateKinematicsChoices();
    void AutoConfigureFromCapabilities(const MachineCapabilities& capabilities);
    
    // Machine data
    std::string m_machineId;
    bool m_isNewMachine;
    EnhancedMachineConfig m_config;
    
    // Main notebook
    wxNotebook* m_notebook;
    
    // Basic tab controls
    wxPanel* m_basicPanel;
    wxTextCtrl* m_nameText;
    wxTextCtrl* m_descriptionText;
    wxTextCtrl* m_hostText;
    wxSpinCtrl* m_portSpinner;
    wxChoice* m_machineTypeChoice;
    wxCheckBox* m_autoConnectCheck;
    
    // Capabilities tab controls
    wxPanel* m_capabilitiesPanel;
    wxButton* m_queryMachineBtn;
    wxButton* m_autoDetectBtn;
    wxStaticText* m_firmwareVersionLabel;
    wxStaticText* m_kinematicsLabel;
    wxStaticText* m_workspaceBoundsLabel;
    wxStaticText* m_featuresLabel;
    wxStaticText* m_lastQueriedLabel;
    wxTextCtrl* m_capabilityDetails;
    
    // Homing tab controls
    wxPanel* m_homingPanel;
    wxCheckBox* m_homingEnabledCheck;
    wxChoice* m_homingSequenceChoice;
    wxSpinCtrlDouble* m_homingFeedRateSpinner;
    wxSpinCtrlDouble* m_homingSeekRateSpinner;
    wxSpinCtrlDouble* m_homingPullOffSpinner;
    
    // Custom sequence editor
    wxStaticBoxSizer* m_customSequenceBox;
    wxListCtrl* m_customSequenceList;
    wxTextCtrl* m_newStepText;
    wxButton* m_addStepBtn;
    wxButton* m_removeStepBtn;
    wxButton* m_moveUpBtn;
    wxButton* m_moveDownBtn;
    wxButton* m_testHomingBtn;
    
    // User settings tab controls
    wxPanel* m_userSettingsPanel;
    wxCheckBox* m_metricUnitsCheck;
    wxSpinCtrlDouble* m_jogFeedRateSpinner;
    wxSpinCtrlDouble* m_jogDistanceSpinner;
    wxCheckBox* m_softLimitsCheck;
    wxCheckBox* m_hardLimitsCheck;
    wxChoice* m_coordinateSystemChoice;
    
    // Dialog buttons
    wxButton* m_okBtn;
    wxButton* m_cancelBtn;
    wxButton* m_applyBtn;
    
    // Progress tracking
    wxProgressDialog* m_progressDialog;
    bool m_queryInProgress;
    bool m_homingInProgress;
    
    // Available choices
    std::vector<wxString> m_machineTypes;
    std::vector<wxString> m_kinematicsTypes;
    std::vector<wxString> m_coordinateSystems;
    
    wxDECLARE_EVENT_TABLE();
};

// Control IDs
enum {
    ID_QUERY_MACHINE = wxID_HIGHEST + 3000,
    ID_TEST_HOMING,
    ID_AUTO_DETECT_KINEMATICS,
    ID_HOMING_SEQUENCE_CHOICE,
    ID_ADD_HOMING_STEP,
    ID_REMOVE_HOMING_STEP,
    ID_MOVE_STEP_UP,
    ID_MOVE_STEP_DOWN,
    ID_CUSTOM_SEQUENCE_LIST,
    ID_APPLY_SETTINGS
};
