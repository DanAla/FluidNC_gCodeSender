/**
 * gui/DROPanel.h
 * Multi-machine Digital Readout display and command interface
 * Shows real-time coordinates, machine status, and provides G-code input
 */

#pragma once

#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/choice.h>
#include <vector>
#include <map>
#include <string>

class ConnectionManager;
class StateManager;

// Simple machine status structure
struct MachineStatus {
    std::string state = "Idle";  // Idle, Run, Hold, Jog, Alarm
    std::vector<float> mpos = {0.0f, 0.0f, 0.0f};  // Machine position
    std::vector<float> wpos = {0.0f, 0.0f, 0.0f};  // Work position
    float feedRate = 0.0f;
    float spindleSpeed = 0.0f;
    std::string coordinateSystem = "G54";
    bool connected = false;
};

/**
 * Professional DRO panel supporting multiple machines
 * Features:
 * - Multi-machine coordinate display
 * - Machine status monitoring
 * - G-code command input with history
 * - Quick command buttons
 * - Coordinate system display (G54-G59)
 */
class DROPanel : public wxPanel
{
public:
    DROPanel(wxWindow* parent, ConnectionManager* connectionManager);
    
    // Machine updates (called from main thread)
    void UpdateMachineStatus(const std::string& machineId, const MachineStatus& status);
    void SetActiveMachine(const std::string& machineId);
    void RefreshDisplay();

private:
    // Event handlers
    void OnMachineChanged(wxCommandEvent& event);
    void OnSendCommand(wxCommandEvent& event);
    void OnCommandEnter(wxCommandEvent& event);
    void OnHistorySelection(wxCommandEvent& event);
    void OnQuickCommand(wxCommandEvent& event);
    void OnZeroWork(wxCommandEvent& event);
    void OnZeroAll(wxCommandEvent& event);
    void OnSetWork(wxCommandEvent& event);
    
    // UI Creation
    void CreateControls();
    void CreateDRODisplay();
    void CreateCommandInterface();
    void CreateQuickCommands();
    
    // Data management
    void LoadCommandHistory();
    void SaveCommandHistory();
    void AddToHistory(const std::string& command);
    void UpdateCoordinateDisplay();
    void UpdateMachineStatusDisplay();
    
    // Core references
    ConnectionManager* m_connectionManager;
    
    // Current machine context
    std::string m_activeMachine;
    std::map<std::string, MachineStatus> m_machineStatuses;
    
    // UI Components - Machine Selection
    wxStaticText* m_machineLabel;
    wxChoice* m_machineChoice;
    wxStaticText* m_connectionStatus;
    
    // UI Components - DRO Display
    wxStaticBoxSizer* m_droSizer;
    
    // Machine Position Display
    wxStaticText* m_mposLabel;
    wxStaticText* m_mposX;
    wxStaticText* m_mposY;
    wxStaticText* m_mposZ;
    wxStaticText* m_mposA;  // Optional 4th axis
    
    // Work Position Display
    wxStaticText* m_wposLabel;
    wxStaticText* m_wposX;
    wxStaticText* m_wposY;
    wxStaticText* m_wposZ;
    wxStaticText* m_wposA;  // Optional 4th axis
    
    // Machine Status Display
    wxStaticText* m_statusLabel;
    wxStaticText* m_machineState;  // Idle, Run, Hold, Jog, Alarm
    wxStaticText* m_feedRate;
    wxStaticText* m_spindleSpeed;
    wxStaticText* m_coordinateSystem;  // G54, G55, etc.
    
    // UI Components - Command Interface
    wxStaticBoxSizer* m_commandSizer;
    wxTextCtrl* m_commandInput;
    wxButton* m_sendButton;
    wxListBox* m_commandHistory;
    
    // UI Components - Quick Commands
    wxStaticBoxSizer* m_quickSizer;
    wxButton* m_homeAllBtn;
    wxButton* m_homeXBtn;
    wxButton* m_homeYBtn;
    wxButton* m_homeZBtn;
    wxButton* m_zeroWorkBtn;
    wxButton* m_zeroAllBtn;
    wxButton* m_setWorkBtn;
    wxButton* m_spindleOnBtn;
    wxButton* m_spindleOffBtn;
    wxButton* m_coolantOnBtn;
    wxButton* m_coolantOffBtn;
    wxButton* m_feedHoldBtn;
    wxButton* m_resumeBtn;
    wxButton* m_resetBtn;
    
    // Formatting and display settings
    int m_coordinateDecimalPlaces = 3;
    bool m_showFourthAxis = false;
    
    wxDECLARE_EVENT_TABLE();
};
