/**
 * gui/ConsolePanel.cpp
 * Console Panel implementation with dummy content and logging
 */

#include "ConsolePanel.h"
#include "MacroConfigDialog.h"
#include "CommunicationManager.h"
#include "NotificationSystem.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/datetime.h>
#include <wx/notebook.h>
#include <wx/filename.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <json.hpp>

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
    ID_COMMAND_HISTORY,
    ID_CONFIGURE_MACROS
};

wxBEGIN_EVENT_TABLE(ConsolePanel, wxPanel)
    EVT_BUTTON(ID_SEND_COMMAND, ConsolePanel::OnSendCommand)
    EVT_TEXT_ENTER(ID_COMMAND_INPUT, ConsolePanel::OnCommandEnter)
    EVT_TEXT(ID_FILTER_TEXT, ConsolePanel::OnFilterChanged)
    EVT_CHECKBOX(ID_SHOW_TIMESTAMPS, ConsolePanel::OnShowTimestamps)
    EVT_CHECKBOX(ID_SHOW_INFO, ConsolePanel::OnShowInfo)
    EVT_CHECKBOX(ID_SHOW_WARNING, ConsolePanel::OnShowWarning)
    EVT_CHECKBOX(ID_SHOW_ERROR, ConsolePanel::OnShowError)
    EVT_CHECKBOX(ID_SHOW_SENT, ConsolePanel::OnShowSent)
    EVT_CHECKBOX(ID_SHOW_RECEIVED, ConsolePanel::OnShowReceived)
    EVT_LISTBOX(ID_COMMAND_HISTORY, ConsolePanel::OnHistorySelected)
    EVT_LISTBOX_DCLICK(ID_COMMAND_HISTORY, ConsolePanel::OnHistoryActivated)
    EVT_BUTTON(ID_CONFIGURE_MACROS, ConsolePanel::OnConfigureMacros)
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
    
    // Initialize console display tracking
    m_displayedEntries = 0;
    
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
    
    filterSizer->AddStretchSpacer();
    
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
    
    // Command input line with macro buttons on the same line
    wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_commandInput = new wxTextCtrl(m_commandPanel, ID_COMMAND_INPUT, wxEmptyString,
                                   wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_sendBtn = new wxButton(m_commandPanel, ID_SEND_COMMAND, "Send");
    
    // Add command input first
    inputSizer->Add(m_commandInput, 1, wxEXPAND | wxRIGHT, 5);
    
    // Create macro buttons panel on the same line
    CreateMacroButtons();
    inputSizer->Add(m_macroPanel, 0, wxALIGN_CENTER_VERTICAL);
    
    // Keep send button for compatibility but hide it
    inputSizer->Add(m_sendBtn, 0);
    m_sendBtn->Show(false);
    
    commandSizer->Add(inputSizer, 0, wxEXPAND | wxALL, 3);
    
    // Disable command input until machine is connected
    m_commandInput->Enable(false);
    m_sendBtn->Enable(false);
    
    // Load and create default macro buttons
    LoadMacroButtons();
    
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
        // If we removed entries, we need to rebuild the display
        m_displayedEntries = 0;
        UpdateLogDisplay();
    } else {
        // Just append the new entry
        AppendNewLogEntry(entry);
    }
    
    // Write to session log if active
    WriteToSessionLog(timestamp, level, message);
}

void ConsolePanel::UpdateLogDisplay()
{
    if (!m_logDisplay) return;
    
    // Clear the display first
    m_logDisplay->Clear();
    m_displayedEntries = 0;
    
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
        
        // Get the color for this level and append colored text
        wxTextAttr attr = GetColorForLevel(entry.level);
        AppendColoredText(line, attr);
        m_displayedEntries++;
    }
    
    // Scroll to bottom
    m_logDisplay->SetInsertionPointEnd();
}

void ConsolePanel::AppendNewLogEntry(const LogEntry& entry)
{
    if (!m_logDisplay) return;
    
    // Check if this entry should be shown
    if (!ShouldShowMessage(entry.level)) return;
    if (!FilterMessage(entry.message).empty() && !m_currentFilter.empty()) {
        if (entry.message.find(m_currentFilter) == std::string::npos) return;
    }
    
    // Format the line
    wxString line;
    
    if (m_showTimestampsFlag) {
        line += "[" + entry.timestamp + "] ";
    }
    
    line += "[" + entry.level + "] " + entry.message + "\n";
    
    // Get the color for this level and append colored text
    wxTextAttr attr = GetColorForLevel(entry.level);
    AppendColoredText(line, attr);
    
    m_displayedEntries++;
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
    m_displayedEntries = 0;
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
        
        // Enable/disable macro buttons
        for (auto& macro : m_macroButtons) {
            if (macro.button) {
                macro.button->Enable(connected);
            }
        }
        
        if (connected) {
            // Start session log when machine connects
            if (!m_activeMachine.empty()) {
                // Use the provided machine name if available, otherwise fall back to machine ID
                std::string displayName = machineName.empty() ? m_activeMachine : machineName;
                StartSessionLog(m_activeMachine, displayName);
            }
            LogMessage("Machine connected - command input and macro buttons enabled", "INFO");
        } else {
            LogMessage("Machine disconnected - command input and macro buttons disabled", "INFO");
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
    
    // Process standard escape sequences
    pos = 0;
    while ((pos = result.find("\\", pos)) != std::string::npos) {
        if (pos + 1 < result.length()) {
            char nextChar = result[pos + 1];
            char replaceChar = '\0';
            
            switch (nextChar) {
                case 'n': replaceChar = '\n'; break;  // newline
                case 't': replaceChar = '\t'; break;  // tab
                case 'r': replaceChar = '\r'; break;  // carriage return
                case '\\': replaceChar = '\\'; break; // backslash
                case '"': replaceChar = '"'; break;   // quote
                case '\'': replaceChar = '\''; break; // apostrophe
                default:
                    // Unknown escape sequence, leave as-is and continue
                    pos += 2;
                    continue;
            }
            
            // Replace the escape sequence with the actual character
            result.replace(pos, 2, 1, replaceChar);
            pos += 1;
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

// Color management for log display
wxTextAttr ConsolePanel::GetColorForLevel(const std::string& level) const
{
    wxTextAttr attr;
    
    // Base monospace font settings
    int baseFontSize = 9;
    wxFontWeight fontWeight = wxFONTWEIGHT_NORMAL;
    
    if (level == "ERROR") {
        attr.SetTextColour(wxColour(255, 85, 85));  // Light red
    } else if (level == "WARN") {
        attr.SetTextColour(wxColour(255, 215, 0));  // Gold/Yellow
    } else if (level == "INFO") {
        attr.SetTextColour(wxColour(135, 206, 235)); // Sky blue
    } else if (level == "SENT") {
        attr.SetTextColour(*wxWHITE); // White
        fontWeight = wxFONTWEIGHT_BOLD; // Bold weight
        baseFontSize = 10; // Slightly larger for better visibility
    } else if (level == "RECV") {
        attr.SetTextColour(wxColour(144, 238, 144)); // Light green for better distinction from SENT
        fontWeight = wxFONTWEIGHT_BOLD; // Bold weight
        baseFontSize = 10; // Slightly larger for better visibility
    } else {
        attr.SetTextColour(*wxWHITE); // Default white
    }
    
    // Create font with appropriate weight and size for this level
    wxFont font(baseFontSize, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, fontWeight);
    attr.SetFont(font);
    
    return attr;
}

void ConsolePanel::AppendColoredText(const wxString& text, const wxTextAttr& attr)
{
    if (!m_logDisplay) return;
    
    // Set insertion point to the end
    m_logDisplay->SetInsertionPointEnd();
    
    // Apply the text attribute and append the text
    m_logDisplay->SetDefaultStyle(attr);
    m_logDisplay->AppendText(text);
    
    // Scroll to bottom
    m_logDisplay->SetInsertionPointEnd();
}

// Macro button implementation
void ConsolePanel::CreateMacroButtons()
{
    m_macroPanel = new wxPanel(m_commandPanel, wxID_ANY);
    wxBoxSizer* macroSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Add label
    macroSizer->Add(new wxStaticText(m_macroPanel, wxID_ANY, "Quick Commands:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    
    // Add configure button
    m_configureMacrosBtn = new wxButton(m_macroPanel, ID_CONFIGURE_MACROS, "Configure...", wxDefaultPosition, wxSize(80, -1));
    
    macroSizer->AddStretchSpacer();
    macroSizer->Add(m_configureMacrosBtn, 0, wxALIGN_CENTER_VERTICAL);
    
    m_macroPanel->SetSizer(macroSizer);
}

void ConsolePanel::LoadMacroButtons()
{
    // Clear existing macro buttons (except configure button)
    m_macroButtons.clear();
    
    // Remove existing macro buttons from sizer
    wxSizer* macroSizer = m_macroPanel->GetSizer();
    
    // Remove all but the first (label) and last (configure button) items
    while (macroSizer->GetItemCount() > 2) {
        wxSizerItem* item = macroSizer->GetItem(1); // Always remove the second item
        if (item && item->IsWindow()) {
            wxWindow* window = item->GetWindow();
            macroSizer->Detach(window);
            window->Destroy();
        } else {
            macroSizer->Remove(1);
        }
    }
    
    // Try to load from configuration file
    std::vector<MacroDefinition> macros;
    if (LoadMacroConfiguration(macros)) {
        // Successfully loaded macros from file
        LogMessage("Loaded " + std::to_string(macros.size()) + " macro configuration(s) from file", "INFO");
        
        // Apply the loaded macros
        int insertPos = 1; // Insert after the label
        bool isConnected = m_commandInput && m_commandInput->IsEnabled();
        
        for (const auto& macroDef : macros) {
            MacroButton macro;
            macro.label = macroDef.label;
            macro.command = macroDef.command;
            macro.description = macroDef.description;
            macro.id = MACRO_BUTTON_BASE_ID + (int)m_macroButtons.size();
            
            // Create the button
            macro.button = new wxButton(m_macroPanel, macro.id, macro.label, 
                                       wxDefaultPosition, wxSize(60, -1));
            macro.button->SetToolTip(macro.description + " (" + macro.command + ")");
            macro.button->Enable(isConnected); // Match current connection state
            
            // Bind the event
            macro.button->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &ConsolePanel::OnMacroButton, this, macro.id);
            
            // Add to sizer
            macroSizer->Insert(insertPos++, macro.button, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3);
            
            // Add to vector
            m_macroButtons.push_back(macro);
        }
        
        // Update layout
        m_macroPanel->Layout();
        m_commandPanel->Layout();
        GetParent()->Layout();
    } else {
        // Failed to load or no configuration exists, create defaults
        LogMessage("No macro configuration found, creating default macros", "INFO");
        ResetMacroButtons();
        // Save the defaults as the new configuration
        std::vector<MacroDefinition> defaultMacros;
        for (const auto& macro : m_macroButtons) {
            MacroDefinition def;
            def.label = macro.label;
            def.command = macro.command;
            def.description = macro.description;
            defaultMacros.push_back(def);
        }
        SaveMacroConfiguration(defaultMacros);
    }
}

void ConsolePanel::ResetMacroButtons()
{
    // Define default macro buttons
    struct DefaultMacro {
        std::string label;
        std::string command;
        std::string description;
    };
    
    std::vector<DefaultMacro> defaultMacros = {
        {"$", "$", "Single status report"},
        {"$$", "$$", "Double status report (detailed)"},
        {"Reset", "\x18", "Soft reset (Ctrl-X)"},
        {"Home", "$H", "Homing cycle"},
        {"Unlock", "$X", "Kill alarm lock"}
    };
    
    // Create macro buttons
    wxSizer* macroSizer = m_macroPanel->GetSizer();
    int insertPos = 1; // Insert after the label
    
    for (const auto& defaultMacro : defaultMacros) {
        MacroButton macro;
        macro.label = defaultMacro.label;
        macro.command = defaultMacro.command;
        macro.description = defaultMacro.description;
        macro.id = MACRO_BUTTON_BASE_ID + (int)m_macroButtons.size();
        
        // Create the button
        macro.button = new wxButton(m_macroPanel, macro.id, macro.label, 
                                   wxDefaultPosition, wxSize(60, -1));
        macro.button->SetToolTip(macro.description + " (" + macro.command + ")");
        macro.button->Enable(false); // Disable until machine connects
        
        // Bind the event
        macro.button->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &ConsolePanel::OnMacroButton, this, macro.id);
        
        // Add to sizer
        macroSizer->Insert(insertPos++, macro.button, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3);
        
        // Add to vector
        m_macroButtons.push_back(macro);
    }
    
    // Update layout
    m_macroPanel->Layout();
    m_commandPanel->Layout();
    GetParent()->Layout();
}

void ConsolePanel::SaveMacroButtons()
{
    // Convert current macro buttons to MacroDefinition vector
    std::vector<MacroDefinition> macros;
    for (const auto& macro : m_macroButtons) {
        MacroDefinition def;
        def.label = macro.label;
        def.command = macro.command;
        def.description = macro.description;
        macros.push_back(def);
    }
    
    // Save to configuration file
    if (SaveMacroConfiguration(macros)) {
        LogMessage("Saved " + std::to_string(macros.size()) + " macro configuration(s) to file", "INFO");
    } else {
        LogError("Failed to save macro configuration to file");
    }
}

void ConsolePanel::OnMacroButton(wxCommandEvent& event)
{
    int buttonId = event.GetId();
    
    // Find the macro button
    for (const auto& macro : m_macroButtons) {
        if (macro.id == buttonId) {
            if (!m_activeMachine.empty()) {
                // Process the command for special characters (including newlines)
                std::string processedCmd = ProcessSpecialCharacters(macro.command);
                
                // Add original command to history
                AddToHistory(macro.command);
                
                // Split the processed command on newlines to handle multiple commands
                std::vector<std::string> commands;
                std::istringstream iss(processedCmd);
                std::string line;
                
                while (std::getline(iss, line)) {
                    // Trim whitespace from each line
                    line.erase(0, line.find_first_not_of(" \t\r"));
                    line.erase(line.find_last_not_of(" \t\r") + 1);
                    
                    if (!line.empty()) {
                        commands.push_back(line);
                    }
                }
                
                // Send each command separately
                bool allSent = true;
                int commandCount = 0;
                
                for (const auto& cmd : commands) {
                    bool sent = CommunicationManager::Instance().SendCommand(m_activeMachine, cmd);
                    
                    if (sent) {
                        commandCount++;
                    } else {
                        allSent = false;
                        LogError("Failed to send command: " + cmd + " (machine not connected)");
                    }
                }
                
                if (allSent && commandCount > 0) {
                    // All commands sent successfully
                    if (commandCount == 1) {
                        LogMessage("Macro button: " + macro.label + " (" + macro.command + ")", "INFO");
                    } else {
                        LogMessage("Macro button: " + macro.label + " (sent " + std::to_string(commandCount) + " commands)", "INFO");
                    }
                } else if (commandCount == 0) {
                    LogError("Failed to send macro command: " + macro.command + " (no valid commands or machine not connected)");
                } else {
                    LogWarning("Macro partially sent: " + std::to_string(commandCount) + " of " + std::to_string(commands.size()) + " commands succeeded");
                }
            } else {
                LogError("No active machine for macro command: " + macro.label);
            }
            break;
        }
    }
}

void ConsolePanel::OnConfigureMacros(wxCommandEvent& WXUNUSED(event))
{
    // Convert current macro buttons to MacroDefinition vector
    std::vector<MacroDefinition> currentMacros;
    for (const auto& macro : m_macroButtons) {
        MacroDefinition def;
        def.label = macro.label;
        def.command = macro.command;
        def.description = macro.description;
        currentMacros.push_back(def);
    }
    
    // Show the macro configuration dialog
    MacroConfigDialog dialog(this, currentMacros);
    if (dialog.ShowModal() == wxID_OK) {
        // Apply the changes from the dialog
        ApplyMacroChanges(dialog.GetMacros());
        
        NotificationSystem::Instance().ShowSuccess(
            "Macros Updated", 
            "Quick command macros have been updated successfully"
        );
    }
}

void ConsolePanel::ApplyMacroChanges(const std::vector<MacroDefinition>& macros)
{
    // Clear existing macro buttons (except configure button)
    m_macroButtons.clear();
    
    // Remove existing macro buttons from sizer
    wxSizer* macroSizer = m_macroPanel->GetSizer();
    
    // Remove all but the first (label) and last (configure button) items
    while (macroSizer->GetItemCount() > 2) {
        wxSizerItem* item = macroSizer->GetItem(1); // Always remove the second item
        if (item && item->IsWindow()) {
            wxWindow* window = item->GetWindow();
            macroSizer->Detach(window);
            window->Destroy();
        } else {
            macroSizer->Remove(1);
        }
    }
    
    // Create new macro buttons from the provided definitions
    int insertPos = 1; // Insert after the label
    bool isConnected = m_commandInput && m_commandInput->IsEnabled(); // Check current connection state
    
    for (const auto& macroDef : macros) {
        MacroButton macro;
        macro.label = macroDef.label;
        macro.command = macroDef.command;
        macro.description = macroDef.description;
        macro.id = MACRO_BUTTON_BASE_ID + (int)m_macroButtons.size();
        
        // Create the button
        macro.button = new wxButton(m_macroPanel, macro.id, macro.label, 
                                   wxDefaultPosition, wxSize(60, -1));
        macro.button->SetToolTip(macro.description + " (" + macro.command + ")");
        macro.button->Enable(isConnected); // Match current connection state
        
        // Bind the event
        macro.button->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &ConsolePanel::OnMacroButton, this, macro.id);
        
        // Add to sizer
        macroSizer->Insert(insertPos++, macro.button, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3);
        
        // Add to vector
        m_macroButtons.push_back(macro);
    }
    
    // Update layout
    m_macroPanel->Layout();
    m_commandPanel->Layout();
    GetParent()->Layout();
    
    // Automatically save the new configuration
    SaveMacroButtons();
}

// Macro configuration persistence implementation
std::string ConsolePanel::GetMacroConfigPath() const
{
    // Create config directory in the application directory
    wxString appDir = wxGetCwd();
    wxString configDir = appDir + wxFILE_SEP_PATH + "config";
    
    // Create config directory if it doesn't exist
    if (!wxDirExists(configDir)) {
        wxMkdir(configDir);
    }
    
    // Return path to macro configuration file
    return (configDir + wxFILE_SEP_PATH + "macros.json").ToStdString();
}

bool ConsolePanel::LoadMacroConfiguration(std::vector<MacroDefinition>& macros) const
{
    std::string configPath = GetMacroConfigPath();
    
    // Check if configuration file exists
    if (!wxFileExists(configPath)) {
        return false; // No configuration file exists
    }
    
    try {
        // Read the configuration file
        std::ifstream configFile(configPath);
        if (!configFile.is_open()) {
            return false;
        }
        
        // Parse JSON
        nlohmann::json root;
        
        try {
            configFile >> root;
        } catch (const std::exception& e) {
            configFile.close();
            return false;
        }
        
        configFile.close();
        
        // Check if it's a valid macro configuration
        if (!root.is_object() || !root.contains("macros") || !root["macros"].is_array()) {
            return false;
        }
        
        // Clear existing macros and load from JSON
        macros.clear();
        
        const nlohmann::json& macroArray = root["macros"];
        for (const auto& macroJson : macroArray) {
            if (macroJson.is_object() &&
                macroJson.contains("label") && macroJson["label"].is_string() &&
                macroJson.contains("command") && macroJson["command"].is_string() &&
                macroJson.contains("description") && macroJson["description"].is_string()) {
                
                MacroDefinition macro;
                macro.label = macroJson["label"].get<std::string>();
                macro.command = macroJson["command"].get<std::string>();
                macro.description = macroJson["description"].get<std::string>();
                
                macros.push_back(macro);
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        // JSON parsing or file I/O error
        return false;
    }
}

bool ConsolePanel::SaveMacroConfiguration(const std::vector<MacroDefinition>& macros) const
{
    std::string configPath = GetMacroConfigPath();
    
    try {
        // Create JSON structure
        nlohmann::json root = nlohmann::json::object();
        nlohmann::json macroArray = nlohmann::json::array();
        
        // Add version info
        root["version"] = "1.0";
        root["description"] = "FluidNC gCode Sender Macro Configuration";
        
        // Convert macros to JSON
        for (const auto& macro : macros) {
            nlohmann::json macroJson = nlohmann::json::object();
            macroJson["label"] = macro.label;
            macroJson["command"] = macro.command;
            macroJson["description"] = macro.description;
            
            macroArray.push_back(macroJson);
        }
        
        root["macros"] = macroArray;
        
        // Write to file with pretty formatting
        std::ofstream configFile(configPath);
        if (!configFile.is_open()) {
            return false;
        }
        
        configFile << root.dump(2); // Pretty print with 2-space indentation
        configFile.close();
        return true;
        
    } catch (const std::exception& e) {
        // JSON creation or file I/O error
        return false;
    }
}
