/**
 * gui/ConsolePanel.cpp
 * Console Panel implementation with dummy content and logging
 */

#include "ConsolePanel.h"
#include "NotificationSystem.h"
#include "../core/CommunicationManager.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/datetime.h>
#include <wx/notebook.h>
#include <wx/filename.h>
#include <algorithm>

// Control IDs
enum {
    ID_CONSOLE_LOG = wxID_HIGHEST + 4000,
    ID_COMMAND_INPUT,
    ID_SEND_COMMAND,
    ID_CLEAR_LOG,
    ID_SAVE_LOG,
    ID_FILTER_TEXT,
    ID_SHOW_TIMESTAMPS,
    ID_SHOW_INFO,
    ID_SHOW_WARNING,
    ID_SHOW_ERROR,
    ID_SHOW_SENT,
    ID_SHOW_RECEIVED,
    ID_COMMAND_HISTORY
};

wxBEGIN_EVENT_TABLE(ConsolePanel, wxPanel)
    EVT_BUTTON(ID_SEND_COMMAND, ConsolePanel::OnSendCommand)
    EVT_TEXT_ENTER(ID_COMMAND_INPUT, ConsolePanel::OnCommandEnter)
    EVT_BUTTON(ID_CLEAR_LOG, ConsolePanel::OnClearLog)
    EVT_BUTTON(ID_SAVE_LOG, ConsolePanel::OnSaveLog)
    EVT_TEXT(ID_FILTER_TEXT, ConsolePanel::OnFilterChanged)
    EVT_CHECKBOX(ID_SHOW_TIMESTAMPS, ConsolePanel::OnShowTimestamps)
    EVT_CHECKBOX(ID_SHOW_INFO, ConsolePanel::OnShowInfo)
    EVT_CHECKBOX(ID_SHOW_WARNING, ConsolePanel::OnShowWarning)
    EVT_CHECKBOX(ID_SHOW_ERROR, ConsolePanel::OnShowError)
    EVT_CHECKBOX(ID_SHOW_SENT, ConsolePanel::OnShowSent)
    EVT_CHECKBOX(ID_SHOW_RECEIVED, ConsolePanel::OnShowReceived)
    EVT_LISTBOX(ID_COMMAND_HISTORY, ConsolePanel::OnHistorySelected)
    EVT_LISTBOX_DCLICK(ID_COMMAND_HISTORY, ConsolePanel::OnHistoryActivated)
    EVT_CHAR_HOOK(ConsolePanel::OnKeyDown)
wxEND_EVENT_TABLE()

ConsolePanel::ConsolePanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), m_splitter(nullptr)
{
    // Initialize display settings
    m_showTimestampsFlag = true;
    m_showInfoFlag = true;
    m_showWarningFlag = true;
    m_showErrorFlag = true;
    m_showSentFlag = true;
    m_showReceivedFlag = true;
    
    // Initialize command history navigation
    m_historyIndex = -1;
    m_historyExpanded = false;
    m_currentCommand = "";
    
    // Initialize session logging
    m_sessionLogActive = false;
    m_sessionMachineId = "";
    m_sessionMachineName = "";
    m_sessionStartTime = "";
    m_sessionLogPath = "";
    
    CreateControls();
    LoadCommandHistory();
    
    // Initialize with clean terminal - ready for real machine communication
    LogMessage("Terminal Console initialized - ready for machine connection", "INFO");
    LogMessage("Select a machine in Machine Manager and connect to begin communication", "INFO");
}

ConsolePanel::~ConsolePanel()
{
    // Ensure session log is properly closed on destruction
    StopSessionLog();
}

void ConsolePanel::CreateControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    wxStaticText* title = new wxStaticText(this, wxID_ANY, "Terminal - Live Communication Monitor");
    title->SetFont(title->GetFont().Scale(1.2).Bold());
    mainSizer->Add(title, 0, wxALL | wxCENTER, 5);
    
    // Create filter controls
    CreateFilterControls();
    mainSizer->Add(m_filterPanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    
    // Create log display (takes most of the space)
    CreateLogDisplay();
    mainSizer->Add(m_logPanel, 1, wxALL | wxEXPAND, 5);
    
    // Create command input area at bottom
    CreateCommandInterface();
    mainSizer->Add(m_commandPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    SetSizer(mainSizer);
}

void ConsolePanel::CreateFilterControls()
{
    m_filterPanel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* filterSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Filter text
    filterSizer->Add(new wxStaticText(m_filterPanel, wxID_ANY, "Filter:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_filterText = new wxTextCtrl(m_filterPanel, ID_FILTER_TEXT, wxEmptyString, wxDefaultPosition, wxSize(150, -1));
    filterSizer->Add(m_filterText, 0, wxRIGHT, 10);
    
    // Checkboxes for message types
    m_showTimestamps = new wxCheckBox(m_filterPanel, ID_SHOW_TIMESTAMPS, "Timestamps");
    m_showTimestamps->SetValue(m_showTimestampsFlag);
    filterSizer->Add(m_showTimestamps, 0, wxRIGHT, 10);
    
    m_showInfo = new wxCheckBox(m_filterPanel, ID_SHOW_INFO, "Info");
    m_showInfo->SetValue(m_showInfoFlag);
    filterSizer->Add(m_showInfo, 0, wxRIGHT, 5);
    
    m_showWarning = new wxCheckBox(m_filterPanel, ID_SHOW_WARNING, "Warn");
    m_showWarning->SetValue(m_showWarningFlag);
    filterSizer->Add(m_showWarning, 0, wxRIGHT, 5);
    
    m_showError = new wxCheckBox(m_filterPanel, ID_SHOW_ERROR, "Error");
    m_showError->SetValue(m_showErrorFlag);
    filterSizer->Add(m_showError, 0, wxRIGHT, 5);
    
    m_showSent = new wxCheckBox(m_filterPanel, ID_SHOW_SENT, "Sent");
    m_showSent->SetValue(m_showSentFlag);
    filterSizer->Add(m_showSent, 0, wxRIGHT, 5);
    
    m_showReceived = new wxCheckBox(m_filterPanel, ID_SHOW_RECEIVED, "Received");
    m_showReceived->SetValue(m_showReceivedFlag);
    filterSizer->Add(m_showReceived, 0, wxRIGHT, 10);
    
    // Control buttons
    m_clearBtn = new wxButton(m_filterPanel, ID_CLEAR_LOG, "Clear");
    m_saveBtn = new wxButton(m_filterPanel, ID_SAVE_LOG, "Save Log");
    
    filterSizer->AddStretchSpacer();
    filterSizer->Add(m_clearBtn, 0, wxRIGHT, 5);
    filterSizer->Add(m_saveBtn, 0);
    
    m_filterPanel->SetSizer(filterSizer);
}

void ConsolePanel::CreateLogDisplay()
{
    m_logPanel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* logSizer = new wxBoxSizer(wxVERTICAL);
    
    // Log display
    m_logDisplay = new wxTextCtrl(m_logPanel, ID_CONSOLE_LOG, wxEmptyString,
                                 wxDefaultPosition, wxDefaultSize,
                                 wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxHSCROLL);
    
    // Use monospace font for log display
    wxFont font(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    m_logDisplay->SetFont(font);
    m_logDisplay->SetBackgroundColour(*wxBLACK);
    m_logDisplay->SetForegroundColour(*wxWHITE);
    
    logSizer->Add(m_logDisplay, 1, wxEXPAND, 0);
    
    m_logPanel->SetSizer(logSizer);
}

void ConsolePanel::CreateCommandInterface()
{
    m_commandPanel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* commandSizer = new wxBoxSizer(wxVERTICAL);
    
    // Command history (initially hidden)
    m_commandHistory = new wxListBox(m_commandPanel, ID_COMMAND_HISTORY);
    m_commandHistory->Show(false); // Initially hidden
    commandSizer->Add(m_commandHistory, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 3);
    
    // Command input line at the bottom
    wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_commandInput = new wxTextCtrl(m_commandPanel, ID_COMMAND_INPUT, wxEmptyString,
                                   wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_sendBtn = new wxButton(m_commandPanel, ID_SEND_COMMAND, "Send");
    
    inputSizer->Add(m_commandInput, 1, wxEXPAND | wxRIGHT, 5);
    inputSizer->Add(m_sendBtn, 0);
    
    commandSizer->Add(inputSizer, 0, wxEXPAND | wxALL, 3);
    
    // Disable command input until machine is connected
    m_commandInput->Enable(false);
    m_sendBtn->Enable(false);
    
    m_commandPanel->SetSizer(commandSizer);
}

std::string ConsolePanel::GetTimestamp() const
{
    wxDateTime now = wxDateTime::Now();
    return now.Format("%H:%M:%S").ToStdString();
}

void ConsolePanel::AddLogEntry(const std::string& timestamp, const std::string& level, 
                              const std::string& message)
{
    LogEntry entry;
    entry.timestamp = timestamp;
    entry.level = level;
    entry.message = message;
    entry.machineId = m_currentMachine;
    
    m_logEntries.push_back(entry);
    
    // Limit log entries to prevent memory issues
    if (m_logEntries.size() > MAX_LOG_ENTRIES) {
        m_logEntries.pop_front();
    }
    
    // Write to session log if active
    WriteToSessionLog(timestamp, level, message);
    
    UpdateLogDisplay();
}

void ConsolePanel::UpdateLogDisplay()
{
    if (!m_logDisplay) return;
    
    wxString displayText;
    
    for (const auto& entry : m_logEntries) {
        if (!ShouldShowMessage(entry.level)) continue;
        if (!FilterMessage(entry.message).empty() && !m_currentFilter.empty()) {
            if (entry.message.find(m_currentFilter) == std::string::npos) continue;
        }
        
        wxString line;
        
        if (m_showTimestampsFlag) {
            line += "[" + entry.timestamp + "] ";
        }
        
        line += "[" + entry.level + "] " + entry.message + "\n";
        
        displayText += line;
    }
    
    m_logDisplay->SetValue(displayText);
    
    // Scroll to bottom
    m_logDisplay->SetInsertionPointEnd();
}

bool ConsolePanel::ShouldShowMessage(const std::string& level) const
{
    if (level == "INFO" && !m_showInfoFlag) return false;
    if (level == "WARN" && !m_showWarningFlag) return false;
    if (level == "ERROR" && !m_showErrorFlag) return false;
    if (level == "SENT" && !m_showSentFlag) return false;
    if (level == "RECV" && !m_showReceivedFlag) return false;
    return true;
}

std::string ConsolePanel::FilterMessage(const std::string& message) const
{
    return message; // Simple pass-through for now
}

void ConsolePanel::LoadCommandHistory()
{
    // Load command history from persistent storage
    // TODO: Implement command history persistence
    // For now, start with empty history
    
    // Update UI
    if (m_commandHistory) {
        for (const auto& cmd : m_commandHistoryData) {
            m_commandHistory->Append(cmd);
        }
    }
}

void ConsolePanel::AddToHistory(const std::string& command)
{
    // Remove duplicates
    auto it = std::find(m_commandHistoryData.begin(), m_commandHistoryData.end(), command);
    if (it != m_commandHistoryData.end()) {
        m_commandHistoryData.erase(it);
    }
    
    // Add to front
    m_commandHistoryData.insert(m_commandHistoryData.begin(), command);
    
    // Limit history size
    if (m_commandHistoryData.size() > MAX_COMMAND_HISTORY) {
        m_commandHistoryData.resize(MAX_COMMAND_HISTORY);
    }
    
    // Update UI
    if (m_commandHistory) {
        m_commandHistory->Clear();
        for (const auto& cmd : m_commandHistoryData) {
            m_commandHistory->Append(cmd);
        }
    }
}

// Public interface methods
void ConsolePanel::LogMessage(const std::string& message, const std::string& level)
{
    AddLogEntry(GetTimestamp(), level, message);
}

void ConsolePanel::LogSentCommand(const std::string& command)
{
    AddLogEntry(GetTimestamp(), "SENT", "> " + command);
}

void ConsolePanel::LogReceivedResponse(const std::string& response)
{
    AddLogEntry(GetTimestamp(), "RECV", "< " + response);
}

void ConsolePanel::LogError(const std::string& error)
{
    AddLogEntry(GetTimestamp(), "ERROR", error);
}

void ConsolePanel::LogWarning(const std::string& warning)
{
    AddLogEntry(GetTimestamp(), "WARN", warning);
}

void ConsolePanel::ClearLog()
{
    m_logEntries.clear();
    UpdateLogDisplay();
}

void ConsolePanel::SaveLog()
{
    wxFileDialog dialog(this, "Save console log", "", "console.log",
                       "Log files (*.log)|*.log|Text files (*.txt)|*.txt|All files (*.*)|*.*",
                       wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dialog.ShowModal() == wxID_OK) {
        NotificationSystem::Instance().ShowInfo(
            "Save Log",
            wxString::Format("Would save log to: %s (%zu entries)", dialog.GetPath(), m_logEntries.size())
        );
    }
}

void ConsolePanel::SetMachine(const std::string& machineId)
{
    m_currentMachine = machineId;
    LogMessage("Switched to machine: " + machineId, "INFO");
}

void ConsolePanel::SetActiveMachine(const std::string& machineId, const std::string& machineName)
{
    m_activeMachine = machineId;
    std::string displayName = machineName.empty() ? machineId : machineName;
    LogMessage("Active machine for commands: " + displayName, "INFO");
    
    // Store the machine name for use in logging
    m_currentMachineName = displayName;
}

void ConsolePanel::SetFilter(const std::string& filter)
{
    m_currentFilter = filter;
    UpdateLogDisplay();
}

void ConsolePanel::SetShowTimestamps(bool show)
{
    m_showTimestampsFlag = show;
    if (m_showTimestamps) {
        m_showTimestamps->SetValue(show);
    }
    UpdateLogDisplay();
}

void ConsolePanel::SetShowLevel(const std::string& level, bool show)
{
    if (level == "INFO") m_showInfoFlag = show;
    else if (level == "WARN") m_showWarningFlag = show;
    else if (level == "ERROR") m_showErrorFlag = show;
    else if (level == "SENT") m_showSentFlag = show;
    else if (level == "RECV") m_showReceivedFlag = show;
    
    UpdateLogDisplay();
}

void ConsolePanel::SetConnectionEnabled(bool connected, const std::string& machineName)
{
    if (m_commandInput && m_sendBtn) {
        m_commandInput->Enable(connected);
        m_sendBtn->Enable(connected);
        
        if (connected) {
            // Start session log when machine connects
            if (!m_activeMachine.empty()) {
                // Use the provided machine name if available, otherwise fall back to machine ID
                std::string displayName = machineName.empty() ? m_activeMachine : machineName;
                StartSessionLog(m_activeMachine, displayName);
            }
            LogMessage("Machine connected - command input enabled", "INFO");
        } else {
            LogMessage("Machine disconnected - command input disabled", "INFO");
            // Stop session log when machine disconnects
            StopSessionLog();
        }
    }
}

// Event handlers
void ConsolePanel::OnSendCommand(wxCommandEvent& WXUNUSED(event))
{
    wxString command = m_commandInput->GetValue().Trim();
    if (!command.IsEmpty() && !m_activeMachine.empty()) {
        std::string cmdStr = command.ToStdString();
        
        // Process escape sequences and special characters
        std::string processedCmd = ProcessSpecialCharacters(cmdStr);
        
        // Add original command to history
        AddToHistory(cmdStr);
        
        // Send the processed command through CommunicationManager
        bool sent = CommunicationManager::Instance().SendCommand(m_activeMachine, processedCmd);
        
        if (sent) {
            // Command was sent successfully - the CommunicationManager will handle logging via callbacks
            // Note: The actual sent command will be logged by the communication manager callback
        } else {
            // Failed to send command
            LogError("Failed to send command: " + cmdStr + " (machine not connected or not found)");
        }
        
        m_commandInput->Clear();
    } else if (m_activeMachine.empty()) {
        LogError("No active machine selected for commands");
    }
}

void ConsolePanel::OnCommandEnter(wxCommandEvent& event)
{
    OnSendCommand(event);
}

void ConsolePanel::OnClearLog(wxCommandEvent& WXUNUSED(event))
{
    int result = wxMessageBox("Are you sure you want to clear the console log?",
                             "Clear Log", wxYES_NO | wxICON_QUESTION, this);
    
    if (result == wxYES) {
        ClearLog();
    }
}

void ConsolePanel::OnSaveLog(wxCommandEvent& WXUNUSED(event))
{
    SaveLog();
}

void ConsolePanel::OnFilterChanged(wxCommandEvent& WXUNUSED(event))
{
    m_currentFilter = m_filterText->GetValue().ToStdString();
    UpdateLogDisplay();
}

void ConsolePanel::OnShowTimestamps(wxCommandEvent& WXUNUSED(event))
{
    m_showTimestampsFlag = m_showTimestamps->GetValue();
    UpdateLogDisplay();
}

void ConsolePanel::OnShowInfo(wxCommandEvent& WXUNUSED(event))
{
    m_showInfoFlag = m_showInfo->GetValue();
    UpdateLogDisplay();
}

void ConsolePanel::OnShowWarning(wxCommandEvent& WXUNUSED(event))
{
    m_showWarningFlag = m_showWarning->GetValue();
    UpdateLogDisplay();
}

void ConsolePanel::OnShowError(wxCommandEvent& WXUNUSED(event))
{
    m_showErrorFlag = m_showError->GetValue();
    UpdateLogDisplay();
}

void ConsolePanel::OnShowSent(wxCommandEvent& WXUNUSED(event))
{
    m_showSentFlag = m_showSent->GetValue();
    UpdateLogDisplay();
}

void ConsolePanel::OnShowReceived(wxCommandEvent& WXUNUSED(event))
{
    m_showReceivedFlag = m_showReceived->GetValue();
    UpdateLogDisplay();
}

void ConsolePanel::OnHistorySelected(wxCommandEvent& WXUNUSED(event))
{
    int selection = m_commandHistory->GetSelection();
    if (selection != wxNOT_FOUND && selection < (int)m_commandHistoryData.size()) {
        m_commandInput->SetValue(m_commandHistoryData[selection]);
    }
}

void ConsolePanel::OnHistoryActivated(wxCommandEvent& WXUNUSED(event))
{
    wxCommandEvent evt;
    OnHistorySelected(evt);
    OnSendCommand(evt);
}

void ConsolePanel::OnKeyDown(wxKeyEvent& event)
{
    if (event.GetEventObject() != m_commandInput) {
        event.Skip();
        return;
    }
    
    int keyCode = event.GetKeyCode();
    
    // Handle control characters for telnet terminal functionality
    if (event.ControlDown() && !m_activeMachine.empty()) {
        char controlChar = 0;
        std::string description;
        
        switch (keyCode) {
            case 'X':
            case 'x':
                controlChar = 24; // CTRL-X (Cancel/Reset)
                description = "CTRL-X (Reset)";
                break;
            case 'C':
            case 'c':
                controlChar = 3; // CTRL-C (Break)
                description = "CTRL-C (Break)";
                break;
            case 'Z':
            case 'z':
                controlChar = 26; // CTRL-Z (Suspend)
                description = "CTRL-Z (Suspend)";
                break;
            case 'D':
            case 'd':
                controlChar = 4; // CTRL-D (EOF)
                description = "CTRL-D (EOF)";
                break;
        }
        
        if (controlChar != 0) {
            // Send the control character directly
            std::string controlStr(1, controlChar);
            bool sent = CommunicationManager::Instance().SendCommand(m_activeMachine, controlStr);
            
            if (sent) {
                LogMessage("Sent: " + description, "INFO");
            } else {
                LogError("Failed to send " + description + " (machine not connected)");
            }
            return; // Don't process further
        }
    }
    
    if (keyCode == WXK_UP) {
        // Show history if not already shown and navigate up
        if (!m_historyExpanded) {
            ShowCommandHistory(true);
        }
        
        if (!m_commandHistoryData.empty()) {
            if (m_historyIndex == -1) {
                // First time pressing up - save current input and move to most recent
                m_currentCommand = m_commandInput->GetValue().ToStdString();
                m_historyIndex = 0;
            } else if (m_historyIndex < (int)m_commandHistoryData.size() - 1) {
                m_historyIndex++;
            }
            
            // Update input with history command
            m_commandInput->SetValue(m_commandHistoryData[m_historyIndex]);
            m_commandInput->SetInsertionPointEnd();
            
            // Select the item in history list
            if (m_commandHistory->IsShown()) {
                m_commandHistory->SetSelection(m_historyIndex);
            }
        }
    }
    else if (keyCode == WXK_DOWN) {
        if (m_historyExpanded && !m_commandHistoryData.empty()) {
            if (m_historyIndex > 0) {
                m_historyIndex--;
                m_commandInput->SetValue(m_commandHistoryData[m_historyIndex]);
                m_commandInput->SetInsertionPointEnd();
                
                // Select the item in history list
                if (m_commandHistory->IsShown()) {
                    m_commandHistory->SetSelection(m_historyIndex);
                }
            } else if (m_historyIndex == 0) {
                // Return to original command
                m_historyIndex = -1;
                m_commandInput->SetValue(m_currentCommand);
                m_commandInput->SetInsertionPointEnd();
                
                // Clear selection
                if (m_commandHistory->IsShown()) {
                    m_commandHistory->SetSelection(wxNOT_FOUND);
                }
                
                // Hide history when we return to original
                ShowCommandHistory(false);
            }
        }
    }
    else if (keyCode == WXK_ESCAPE) {
        // ESC key hides history and resets to original command
        if (m_historyExpanded) {
            ShowCommandHistory(false);
            m_historyIndex = -1;
            m_commandInput->SetValue(m_currentCommand);
            m_commandInput->SetInsertionPointEnd();
        }
    }
    else {
        // Any other key hides history and resets index
        if (m_historyExpanded && keyCode != WXK_RETURN && keyCode != WXK_NUMPAD_ENTER) {
            ShowCommandHistory(false);
            m_historyIndex = -1;
        }
        event.Skip();
    }
}

void ConsolePanel::ShowCommandHistory(bool show)
{
    if (m_historyExpanded == show) return;
    
    m_historyExpanded = show;
    
    if (show) {
        // Show up to 4 commands
        int numItems = std::min(4, (int)m_commandHistoryData.size());
        if (numItems > 0) {
            // Calculate height needed (item height * num items + some padding)
            int itemHeight = m_commandHistory->GetCharHeight() + 4;
            int historyHeight = itemHeight * numItems + 8;
            
            m_commandHistory->SetMinSize(wxSize(-1, historyHeight));
            m_commandHistory->Show(true);
            
            // Update sizer to accommodate new size
            m_commandPanel->GetSizer()->Layout();
            
            // Refresh parent to update layout
            GetParent()->Layout();
        }
    } else {
        m_commandHistory->Show(false);
        m_commandHistory->SetSelection(wxNOT_FOUND);
        
        // Update layout
        m_commandPanel->GetSizer()->Layout();
        GetParent()->Layout();
    }
}

std::string ConsolePanel::ProcessSpecialCharacters(const std::string& input) const
{
    std::string result = input;
    
    // Process escape sequences and special character notations
    size_t pos = 0;
    while ((pos = result.find("\\x", pos)) != std::string::npos) {
        if (pos + 3 < result.length()) {
            // Extract hex value (2 characters after \x)
            std::string hexStr = result.substr(pos + 2, 2);
            char* endPtr;
            long hexValue = std::strtol(hexStr.c_str(), &endPtr, 16);
            
            if (*endPtr == '\0' && hexValue >= 0 && hexValue <= 255) {
                // Valid hex value, replace \xHH with actual character
                char actualChar = static_cast<char>(hexValue);
                result.replace(pos, 4, 1, actualChar);
                pos += 1;
            } else {
                pos += 2;
            }
        } else {
            break;
        }
    }
    
    // Process caret notation (^X = CTRL-X)
    pos = 0;
    while ((pos = result.find("^", pos)) != std::string::npos) {
        if (pos + 1 < result.length()) {
            char nextChar = result[pos + 1];
            if (nextChar >= 'A' && nextChar <= 'Z') {
                // Convert ^A to CTRL-A (ASCII 1), ^B to CTRL-B (ASCII 2), etc.
                char controlChar = nextChar - 'A' + 1;
                result.replace(pos, 2, 1, controlChar);
                pos += 1;
            } else if (nextChar >= 'a' && nextChar <= 'z') {
                // Convert ^a to CTRL-A (ASCII 1), ^b to CTRL-B (ASCII 2), etc.
                char controlChar = nextChar - 'a' + 1;
                result.replace(pos, 2, 1, controlChar);
                pos += 1;
            } else {
                pos += 1;
            }
        } else {
            break;
        }
    }
    
    return result;
}

void ConsolePanel::SaveCommandHistory()
{
    // TODO: Implement command history persistence
}

// Session logging implementation
void ConsolePanel::StartSessionLog(const std::string& machineId, const std::string& machineName)
{
    // Stop any existing session log
    StopSessionLog();
    
    m_sessionMachineId = machineId;
    m_sessionMachineName = machineName.empty() ? machineId : machineName;
    
    // Get current timestamp for session start and filename
    wxDateTime now = wxDateTime::Now();
    m_sessionStartTime = now.Format("%Y-%m-%d_%H-%M-%S").ToStdString();
    
    // Create session log file path
    m_sessionLogPath = GetSessionLogPath(m_sessionMachineName, m_sessionStartTime);
    
    // Create logs directory if it doesn't exist
    wxString logDir = wxFileName(m_sessionLogPath).GetPath();
    if (!wxDirExists(logDir)) {
        wxMkdir(logDir);
    }
    
    // Open the session log file
    m_sessionLogFile.open(m_sessionLogPath, std::ios::out | std::ios::app);
    
    if (m_sessionLogFile.is_open()) {
        m_sessionLogActive = true;
        
        // Write session header
        std::string sessionHeader = "=== FluidNC Terminal Session Log ===\n";
        sessionHeader += "Machine ID: " + m_sessionMachineId + "\n";
        sessionHeader += "Machine Name: " + m_sessionMachineName + "\n";
        sessionHeader += "Session Started: " + now.Format("%Y-%m-%d %H:%M:%S").ToStdString() + "\n";
        sessionHeader += "=====================================\n\n";
        
        m_sessionLogFile << sessionHeader;
        m_sessionLogFile.flush();
        
        LogMessage("Session log started: " + m_sessionLogPath, "INFO");
    } else {
        LogError("Failed to create session log file: " + m_sessionLogPath);
        m_sessionLogActive = false;
    }
}

void ConsolePanel::StopSessionLog()
{
    if (m_sessionLogActive && m_sessionLogFile.is_open()) {
        // Write session footer
        wxDateTime now = wxDateTime::Now();
        std::string sessionFooter = "\n=====================================\n";
        sessionFooter += "Session Ended: " + now.Format("%Y-%m-%d %H:%M:%S").ToStdString() + "\n";
        sessionFooter += "=== End of FluidNC Terminal Session ===\n";
        
        m_sessionLogFile << sessionFooter;
        m_sessionLogFile.flush();
        m_sessionLogFile.close();
        
        LogMessage("Session log stopped and saved: " + m_sessionLogPath, "INFO");
        
        m_sessionLogActive = false;
        m_sessionLogPath.clear();
        m_sessionMachineId.clear();
        m_sessionMachineName.clear();
        m_sessionStartTime.clear();
    }
}

void ConsolePanel::WriteToSessionLog(const std::string& timestamp, const std::string& level, const std::string& message)
{
    if (m_sessionLogActive && m_sessionLogFile.is_open()) {
        std::string logLine = "[" + timestamp + "] [" + level + "] " + message + "\n";
        m_sessionLogFile << logLine;
        m_sessionLogFile.flush(); // Ensure immediate write for real-time logging
    }
}

std::string ConsolePanel::GetSessionLogPath(const std::string& machineName, const std::string& timestamp) const
{
    // Create logs directory in the application directory
    wxString appDir = wxGetCwd();
    wxString logsDir = appDir + wxFILE_SEP_PATH + "logs";
    
    // Clean machine name for use in filename (remove invalid filename characters)
    std::string cleanMachineName = machineName;
    std::replace(cleanMachineName.begin(), cleanMachineName.end(), ':', '_');
    std::replace(cleanMachineName.begin(), cleanMachineName.end(), '/', '_');
    std::replace(cleanMachineName.begin(), cleanMachineName.end(), '\\', '_');
    std::replace(cleanMachineName.begin(), cleanMachineName.end(), ' ', '-');
    std::replace(cleanMachineName.begin(), cleanMachineName.end(), '<', '_');
    std::replace(cleanMachineName.begin(), cleanMachineName.end(), '>', '_');
    std::replace(cleanMachineName.begin(), cleanMachineName.end(), '|', '_');
    std::replace(cleanMachineName.begin(), cleanMachineName.end(), '?', '_');
    std::replace(cleanMachineName.begin(), cleanMachineName.end(), '*', '_');
    std::replace(cleanMachineName.begin(), cleanMachineName.end(), '"', '_');
    
    // Create filename: MachineName_YYYY-MM-DD_HH-MM-SS.log
    std::string filename = cleanMachineName + "_" + timestamp + ".log";
    
    return (logsDir + wxFILE_SEP_PATH + filename).ToStdString();
}
