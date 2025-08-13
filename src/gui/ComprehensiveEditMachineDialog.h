/**
 * gui/ComprehensiveEditMachineDialog.h
 * Complete FluidNC/GRBL machine configuration dialog with full auto-discovery
 * Multi-tab interface covering ALL machine settings and capabilities
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
#include <wx/grid.h>
#include <wx/splitter.h>
#include "../core/MachineConfigManager.h"
#include "../core/HomingManager.h"
#include "../core/CommunicationManager.h"

/**
 * Comprehensive Edit Machine Dialog - Complete FluidNC/GRBL Configuration
 * Features:
 * - Basic Settings (Connection, Identity)
 * - Auto-Discovery (One-click population of ALL settings from machine)
 * - Motion Settings (Steps/mm, Max Rates, Acceleration, Travel Limits)
 * - Homing Settings (Kinematics-aware sequences, rates, behavior)
 * - Spindle & Coolant (Speed ranges, PWM settings, output pins)
 * - Probe Settings (Pin configuration, travel distances, rates)
 * - Safety & Limits (Hard/Soft limits, alarm behavior)
 * - Pin Configuration (Input/Output mappings, invert settings)
 * - Advanced Settings (Junction deviation, arc tolerance, etc.)
 * - System Info & Build Details (Firmware version, features, memory)
 * - Real-time Testing (Test homing, probe, spindle, etc.)
 */
class ComprehensiveEditMachineDialog : public wxDialog {
public:
    ComprehensiveEditMachineDialog(wxWindow* parent, const std::string& machineId = "", bool isNewMachine = true);
    ~ComprehensiveEditMachineDialog();
    
    // Get the configured machine data
    EnhancedMachineConfig GetMachineConfig() const;
    void SetMachineConfig(const EnhancedMachineConfig& config);
    
private:
    // Event handlers
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnApply(wxCommandEvent& event);
    void OnAutoDiscover(wxCommandEvent& event);  // THE BIG BUTTON!
    void OnTestConnection(wxCommandEvent& event);
    void OnResetToDefaults(wxCommandEvent& event);
    void OnExportConfig(wxCommandEvent& event);
    void OnImportConfig(wxCommandEvent& event);
    
    // Tab creation methods
    void CreateNotebook();
    void CreateBasicTab(wxNotebook* notebook);
    void CreateMotionTab(wxNotebook* notebook);
    void CreateHomingTab(wxNotebook* notebook);
    void CreateSpindleCoolantTab(wxNotebook* notebook);
    void CreateProbeTab(wxNotebook* notebook);
    void CreateSafetyLimitsTab(wxNotebook* notebook);
    void CreatePinConfigTab(wxNotebook* notebook);
    void CreateAdvancedTab(wxNotebook* notebook);
    void CreateSystemInfoTab(wxNotebook* notebook);
    void CreateTestingTab(wxNotebook* notebook);
    
    // Auto-discovery system
    void StartAutoDiscovery();
    void OnDiscoveryProgress(const std::string& message, int progress);
    void OnDiscoveryComplete();
    void OnDiscoveryError(const std::string& error);
    
    // Individual discovery phases
    void DiscoverSystemInfo();
    void DiscoverGRBLSettings();
    void DiscoverBuildInfo();
    void DiscoverPinMappings();
    void DiscoverCapabilities();
    void DiscoverKinematics();
    void AutoConfigureFromDiscovery();
    
    // Settings management
    void LoadAllSettings();
    void SaveAllSettings();
    bool ValidateAllSettings();
    void PopulateGRBLGrid();
    void UpdateGRBLSetting(int parameter, float value, const std::string& description);
    
    // Testing functions
    void TestHoming(wxCommandEvent& event);
    void TestProbe(wxCommandEvent& event);
    void TestSpindle(wxCommandEvent& event);
    void TestJogging(wxCommandEvent& event);
    void TestLimits(wxCommandEvent& event);
    
    // Machine data
    std::string m_machineId;
    bool m_isNewMachine;
    EnhancedMachineConfig m_config;
    std::map<int, float> m_grblSettings;
    std::vector<std::string> m_systemInfo;
    std::map<std::string, std::string> m_pinMappings;
    
    // Discovery state
    bool m_discoveryInProgress;
    wxProgressDialog* m_discoveryProgress;
    std::vector<std::string> m_discoveryLogMessages;
    
    // Main notebook
    wxNotebook* m_notebook;
    
    // Tab 1: Basic Settings
    wxPanel* m_basicPanel;
    wxTextCtrl* m_nameText;
    wxTextCtrl* m_descriptionText;
    wxTextCtrl* m_hostText;
    wxSpinCtrl* m_portSpinner;
    wxChoice* m_machineTypeChoice;
    wxCheckBox* m_autoConnectCheck;
    wxButton* m_autoDiscoverBtn;  // THE BIG BUTTON!
    wxButton* m_testConnectionBtn;
    
    // Tab 2: Motion Settings ($$100-132, etc.)
    wxPanel* m_motionPanel;
    // X/Y/Z Steps per mm
    wxSpinCtrlDouble* m_stepsPerMM_X;
    wxSpinCtrlDouble* m_stepsPerMM_Y;
    wxSpinCtrlDouble* m_stepsPerMM_Z;
    wxSpinCtrlDouble* m_stepsPerMM_A;
    // Max rates
    wxSpinCtrlDouble* m_maxRate_X;
    wxSpinCtrlDouble* m_maxRate_Y;
    wxSpinCtrlDouble* m_maxRate_Z;
    wxSpinCtrlDouble* m_maxRate_A;
    // Acceleration
    wxSpinCtrlDouble* m_acceleration_X;
    wxSpinCtrlDouble* m_acceleration_Y;
    wxSpinCtrlDouble* m_acceleration_Z;
    wxSpinCtrlDouble* m_acceleration_A;
    // Travel limits
    wxSpinCtrlDouble* m_maxTravel_X;
    wxSpinCtrlDouble* m_maxTravel_Y;
    wxSpinCtrlDouble* m_maxTravel_Z;
    wxSpinCtrlDouble* m_maxTravel_A;
    
    // Tab 3: Homing Settings ($$20-30)
    wxPanel* m_homingPanel;
    wxCheckBox* m_homingEnabledCheck;
    wxChoice* m_homingSequenceChoice;
    wxSpinCtrlDouble* m_homingFeedRate;
    wxSpinCtrlDouble* m_homingSeekRate;
    wxSpinCtrlDouble* m_homingDebounce;
    wxSpinCtrlDouble* m_homingPulloff;
    wxCheckBox* m_homingDirInvert_X;
    wxCheckBox* m_homingDirInvert_Y;
    wxCheckBox* m_homingDirInvert_Z;
    wxCheckBox* m_homingDirInvert_A;
    wxListCtrl* m_customHomingList;
    wxButton* m_testHomingBtn;
    
    // Tab 4: Spindle & Coolant
    wxPanel* m_spindleCoolantPanel;
    wxSpinCtrlDouble* m_spindleMaxRPM;
    wxSpinCtrlDouble* m_spindleMinRPM;
    wxChoice* m_spindlePWMFreq;
    wxSpinCtrl* m_spindleOutputPin;
    wxCheckBox* m_spindlePinInvert;
    wxCheckBox* m_coolantFloodEnable;
    wxSpinCtrl* m_coolantFloodPin;
    wxCheckBox* m_coolantMistEnable;
    wxSpinCtrl* m_coolantMistPin;
    wxCheckBox* m_coolantPinInvert;
    wxButton* m_testSpindleBtn;
    
    // Tab 5: Probe Settings
    wxPanel* m_probePanel;
    wxCheckBox* m_probeEnable;
    wxSpinCtrl* m_probePin;
    wxCheckBox* m_probePinInvert;
    wxSpinCtrlDouble* m_probeFeedRate;
    wxSpinCtrlDouble* m_probeSeekRate;
    wxSpinCtrlDouble* m_probeMaxTravel;
    wxSpinCtrlDouble* m_probePulloff;
    wxButton* m_testProbeBtn;
    
    // Tab 6: Safety & Limits
    wxPanel* m_safetyLimitsPanel;
    wxCheckBox* m_softLimitsEnable;
    wxCheckBox* m_hardLimitsEnable;
    wxCheckBox* m_hardLimitsInvert;
    wxChoice* m_hardLimitsDebounce;
    wxCheckBox* m_limitPin_X;
    wxCheckBox* m_limitPin_Y;
    wxCheckBox* m_limitPin_Z;
    wxCheckBox* m_limitPin_A;
    wxChoice* m_alarmBehavior;
    wxSpinCtrlDouble* m_safetyDoorOpenTime;
    wxButton* m_testLimitsBtn;
    
    // Tab 7: Pin Configuration
    wxPanel* m_pinConfigPanel;
    wxGrid* m_pinGrid;
    wxChoice* m_stepperDriver;
    wxCheckBox* m_stepPulseInvert;
    wxCheckBox* m_stepDirInvert;
    wxCheckBox* m_stepEnableInvert;
    wxSpinCtrlDouble* m_stepPulseTime;
    wxSpinCtrlDouble* m_stepIdleDelay;
    
    // Tab 8: Advanced Settings
    wxPanel* m_advancedPanel;
    wxSpinCtrlDouble* m_junctionDeviation;
    wxSpinCtrlDouble* m_arcTolerance;
    wxCheckBox* m_reportInches;
    wxSpinCtrl* m_statusReportMask;
    wxSpinCtrlDouble* m_feedHoldActions;
    wxSpinCtrlDouble* m_softResetActions;
    wxChoice* m_parkingEnable;
    wxSpinCtrlDouble* m_parkingAxis;
    
    // Tab 9: System Info & Build Details
    wxPanel* m_systemInfoPanel;
    wxTextCtrl* m_firmwareVersion;
    wxTextCtrl* m_buildDate;
    wxTextCtrl* m_buildOptions;
    wxTextCtrl* m_systemCapabilities;
    wxListCtrl* m_grblSettingsList;
    wxTextCtrl* m_discoveryLog;
    
    // Tab 10: Real-time Testing
    wxPanel* m_testingPanel;
    wxButton* m_testHomingSequenceBtn;
    wxButton* m_testProbeSequenceBtn;
    wxButton* m_testSpindleControlBtn;
    wxButton* m_testJoggingBtn;
    wxButton* m_testLimitSwitchesBtn;
    wxButton* m_testEmergencyStopBtn;
    wxTextCtrl* m_testResults;
    wxGauge* m_testProgress;
    
    // Dialog buttons
    wxButton* m_okBtn;
    wxButton* m_cancelBtn;
    wxButton* m_applyBtn;
    wxButton* m_resetBtn;
    wxButton* m_exportBtn;
    wxButton* m_importBtn;
    
    // GRBL Parameter definitions (for auto-discovery and validation)
    struct GRBLParameter {
        int number;
        std::string name;
        std::string description;
        std::string unit;
        float defaultValue;
        float minValue;
        float maxValue;
        std::string category;
    };
    std::vector<GRBLParameter> m_grblParameters;
    
    void InitializeGRBLParameters();
    std::string GetParameterDescription(int paramNumber);
    std::string GetParameterUnit(int paramNumber);
    
    wxDECLARE_EVENT_TABLE();
};

// Control IDs for the comprehensive dialog
enum {
    ID_AUTO_DISCOVER = wxID_HIGHEST + 4000,
    ID_TEST_CONNECTION,
    ID_RESET_DEFAULTS,
    ID_EXPORT_CONFIG,
    ID_IMPORT_CONFIG,
    ID_TEST_HOMING_SEQ,
    ID_TEST_PROBE_SEQ,
    ID_TEST_SPINDLE_CTRL,
    ID_TEST_JOGGING,
    ID_TEST_LIMITS,
    ID_TEST_EMERGENCY,
    ID_GRBL_SETTINGS_GRID,
    ID_PIN_CONFIG_GRID,
    ID_CUSTOM_HOMING_LIST
};
