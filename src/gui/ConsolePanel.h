/**
 * gui/ConsolePanel.h
 * Console panel for machine communication and command history
 * Provides terminal-like interface for direct machine communication
 */

#pragma once

#include <wx/wx.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/splitter.h>
#include <vector>
#include <string>
#include <deque>
#include <fstream>

// Forward declarations
struct MacroDefinition;

// Log entry structure
struct LogEntry {
    std::string timestamp;
    std::string level;
    std::string message;
    std::string machineId;
};

/**
 * Terminal Panel - real-time machine communication interface
 * Features:
 * - Live telnet/USB/UART communication monitoring
 * - Real-time command transmission and response logging
 * - Interactive command input with history
 * - Advanced message filtering and search
 * - Connection status monitoring
 * - Export/save communication logs
 * - Multi-protocol support (Telnet, USB, UART)
 * - Professional terminal emulation
 * - Colored output for different message types
 * - Configurable macro buttons for common commands
 */
class ConsolePanel : public wxPanel
{
public:
    ConsolePanel(wxWindow* parent);
    ~ConsolePanel();
    
    // Message logging
    void LogMessage(const std::string& message, const std::string& level = "INFO");
    void LogSentCommand(const std::string& command);
    void LogReceivedResponse(const std::string& response);
    void LogError(const std::string& error);
    void LogWarning(const std::string& warning);
    
    // Console operations
    void ClearLog();
    void SaveLog();
    void SetMachine(const std::string& machineId);
    
    // Filtering
    void SetFilter(const std::string& filter);
    void SetShowTimestamps(bool show);
    void SetShowLevel(const std::string& level, bool show);
    
    // Connection status
    void SetConnectionEnabled(bool connected, const std::string& machineName = "");
    
    // Real communication integration
    void SetActiveMachine(const std::string& machineId, const std::string& machineName = "");
    
    // Macro button management
    void LoadMacroButtons();
    void SaveMacroButtons();
    void ResetMacroButtons();
    void ApplyMacroChanges(const std::vector<MacroDefinition>& macros);

private:
    // Event handlers
    void OnSendCommand(wxCommandEvent& event);
    void OnCommandEnter(wxCommandEvent& event);
    void OnFilterChanged(wxCommandEvent& event);
    void OnShowTimestamps(wxCommandEvent& event);
    void OnShowInfo(wxCommandEvent& event);
    void OnShowWarning(wxCommandEvent& event);
    void OnShowError(wxCommandEvent& event);
    void OnShowSent(wxCommandEvent& event);
    void OnShowReceived(wxCommandEvent& event);
    void OnHistorySelected(wxCommandEvent& event);
    void OnHistoryActivated(wxCommandEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    
    // Macro button event handlers
    void OnMacroButton(wxCommandEvent& event);
    void OnConfigureMacros(wxCommandEvent& event);
    
    // UI Creation
    void CreateControls();
    void CreateLogDisplay();
    void CreateCommandInterface();
    void CreateFilterControls();
    void CreateMacroButtons();
    
    // Color management
    wxTextAttr GetColorForLevel(const std::string& level) const;
    void AppendColoredText(const wxString& text, const wxTextAttr& attr);
    
    // Log management
    void UpdateLogDisplay();
    void AppendNewLogEntry(const LogEntry& entry);
    void AddLogEntry(const std::string& timestamp, const std::string& level, 
                    const std::string& message);
    void LoadCommandHistory();
    void SaveCommandHistory();
    void AddToHistory(const std::string& command);
    std::string GetTimestamp() const;
    
    // Command history navigation
    void ShowCommandHistory(bool show);
    
    // Message filtering
    bool ShouldShowMessage(const std::string& level) const;
    std::string FilterMessage(const std::string& message) const;
    
    // Special character processing for terminal functionality
    std::string ProcessSpecialCharacters(const std::string& input) const;
    
    // Session logging
    void StartSessionLog(const std::string& machineId, const std::string& machineName = "");
    void StopSessionLog();
    void WriteToSessionLog(const std::string& timestamp, const std::string& level, const std::string& message);
    std::string GetSessionLogPath(const std::string& machineName, const std::string& timestamp) const;
    
    // Macro configuration persistence
    std::string GetMacroConfigPath() const;
    bool LoadMacroConfiguration(std::vector<MacroDefinition>& macros) const;
    bool SaveMacroConfiguration(const std::vector<MacroDefinition>& macros) const;
    
    // Macro button structure
    struct MacroButton {
        std::string label;
        std::string command;
        std::string description;
        wxButton* button;
        int id;
    };
    
    // UI Components
    wxSplitterWindow* m_splitter;
    
    // Log display panel
    wxPanel* m_logPanel;
    wxTextCtrl* m_logDisplay;
    wxPanel* m_filterPanel;
    wxTextCtrl* m_filterText;
    wxCheckBox* m_showTimestamps;
    wxCheckBox* m_showInfo;
    wxCheckBox* m_showWarning;
    wxCheckBox* m_showError;
    wxCheckBox* m_showSent;
    wxCheckBox* m_showReceived;
    
    // Command interface panel
    wxPanel* m_commandPanel;
    wxTextCtrl* m_commandInput;
    wxButton* m_sendBtn;  // Keep for compatibility, but will be hidden
    wxListBox* m_commandHistory;
    
    // Macro button panel
    wxPanel* m_macroPanel;
    wxButton* m_configureMacrosBtn;
    
    // Data
    std::deque<LogEntry> m_logEntries;
    std::vector<std::string> m_commandHistoryData;
    std::string m_currentMachine;
    std::string m_activeMachine;  // Currently active machine for sending commands
    std::string m_currentMachineName;  // Human-readable name of current machine
    std::string m_currentFilter;
    
    // Display settings
    bool m_showTimestampsFlag;
    bool m_showInfoFlag;
    bool m_showWarningFlag;
    bool m_showErrorFlag;
    bool m_showSentFlag;
    bool m_showReceivedFlag;
    
    // Command history navigation
    int m_historyIndex;
    bool m_historyExpanded;
    std::string m_currentCommand;
    
    // Session logging
    std::ofstream m_sessionLogFile;
    std::string m_sessionLogPath;
    std::string m_sessionMachineId;
    std::string m_sessionMachineName;
    std::string m_sessionStartTime;
    bool m_sessionLogActive;
    
    // Macro buttons
    std::vector<MacroButton> m_macroButtons;
    static const int MACRO_BUTTON_BASE_ID = 5000;
    
    // Console display tracking
    size_t m_displayedEntries;
    
    // Limits
    static const size_t MAX_LOG_ENTRIES = 1000;
    static const size_t MAX_COMMAND_HISTORY = 50;
    
    wxDECLARE_EVENT_TABLE();
};
