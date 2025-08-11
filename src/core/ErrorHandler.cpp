/**
 * core/ErrorHandler.cpp
 * Implementation of comprehensive error handling system
 */

#include "ErrorHandler.h"
#include <wx/clipbrd.h>
#include <wx/datetime.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/artprov.h>
#include <wx/utils.h>
#include <wx/timer.h>

// Control IDs for ErrorDialog
enum {
    ID_COPY_TO_CLIPBOARD = wxID_HIGHEST + 5000,
    ID_ERROR_DETAILS
};

// ErrorDialog event table
wxBEGIN_EVENT_TABLE(ErrorDialog, wxDialog)
    EVT_BUTTON(ID_COPY_TO_CLIPBOARD, ErrorDialog::OnCopyToClipboard)
    EVT_BUTTON(wxID_OK, ErrorDialog::OnClose)
wxEND_EVENT_TABLE()

// Global assertion handler function
void wxAssertHandler(const wxString& file, int line, const wxString& func,
                    const wxString& cond, const wxString& msg)
{
    CustomAssertHandler::HandleAssertion(file, line, func, cond, msg);
}

// ErrorHandler Implementation
ErrorHandler& ErrorHandler::Instance()
{
    static ErrorHandler instance;
    return instance;
}

void ErrorHandler::Initialize()
{
    // Set up custom log target
    // Note: Keeping wxLog for system-level error handling, but routing through our logger
    wxLog::SetActiveTarget(new CustomLogTarget());
    
    // Set up custom assertion handler
    wxSetAssertHandler(wxAssertHandler);
    
    // Enable assertion handling
    EnableAssertionHandling(true);
}

void ErrorHandler::ReportError(const wxString& title, const wxString& message, const wxString& details)
{
    wxString timestamp = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S");
    wxString fullError = wxString::Format("[%s] ERROR: %s - %s", timestamp, title, message);
    
    if (!details.IsEmpty()) {
        fullError += "\nDetails: " + details;
    }
    
    // Store error
    m_recentErrors.push_back(fullError);
    if (m_recentErrors.size() > MAX_STORED_ERRORS) {
        m_recentErrors.erase(m_recentErrors.begin());
    }
    
    // Show dialog
    ShowErrorDialog(title, message, details, wxICON_ERROR);
}

void ErrorHandler::ReportWarning(const wxString& title, const wxString& message, const wxString& details)
{
    wxString timestamp = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S");
    wxString fullError = wxString::Format("[%s] WARNING: %s - %s", timestamp, title, message);
    
    if (!details.IsEmpty()) {
        fullError += "\nDetails: " + details;
    }
    
    m_recentErrors.push_back(fullError);
    if (m_recentErrors.size() > MAX_STORED_ERRORS) {
        m_recentErrors.erase(m_recentErrors.begin());
    }
    
    ShowErrorDialog(title, message, details, wxICON_WARNING);
}

void ErrorHandler::ReportInfo(const wxString& title, const wxString& message, const wxString& details)
{
    wxString timestamp = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S");
    wxString fullError = wxString::Format("[%s] INFO: %s - %s", timestamp, title, message);
    
    if (!details.IsEmpty()) {
        fullError += "\nDetails: " + details;
    }
    
    m_recentErrors.push_back(fullError);
    if (m_recentErrors.size() > MAX_STORED_ERRORS) {
        m_recentErrors.erase(m_recentErrors.begin());
    }
    
    ShowErrorDialog(title, message, details, wxICON_INFORMATION);
}

void ErrorHandler::EnableAssertionHandling(bool enable)
{
    m_assertionHandlingEnabled = enable;
}

std::vector<wxString> ErrorHandler::GetRecentErrors() const
{
    return m_recentErrors;
}

void ErrorHandler::ClearErrors()
{
    m_recentErrors.clear();
}

void ErrorHandler::ShowErrorDialog(const wxString& title, const wxString& message, 
                                  const wxString& details, int iconType)
{
    // Try to find a parent window
    wxWindow* parent = nullptr;
    wxWindow* topWindow = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
    if (topWindow && topWindow->IsShown()) {
        parent = topWindow;
    }
    
    ErrorDialog dialog(parent, title, message, details, iconType);
    dialog.ShowModal();
}

// CustomLogTarget Implementation
CustomLogTarget::CustomLogTarget()
    : wxLog()
{
}

CustomLogTarget::~CustomLogTarget()
{
}

void CustomLogTarget::DoLogRecord(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info)
{
    wxString levelStr;
    int iconType = wxICON_INFORMATION;
    
    switch (level) {
        case wxLOG_FatalError:
        case wxLOG_Error:
            levelStr = "ERROR";
            iconType = wxICON_ERROR;
            break;
        case wxLOG_Warning:
            levelStr = "WARNING";
            iconType = wxICON_WARNING;
            break;
        case wxLOG_Message:
        case wxLOG_Info:
            levelStr = "INFO";
            iconType = wxICON_INFORMATION;
            break;
        case wxLOG_Debug:
            levelStr = "DEBUG";
            iconType = wxICON_INFORMATION;
            break;
        default:
            levelStr = "LOG";
            iconType = wxICON_INFORMATION;
            break;
    }
    
    // For serious errors, show dialog
    if (level <= wxLOG_Warning) {
        wxString details = wxString::Format("File: %s\nLine: %d\nFunction: %s\nComponent: %s",
                                           info.filename, info.line, info.func, info.component);
        
        ErrorHandler::Instance().ShowErrorDialog(levelStr, msg, details, iconType);
    }
    
    // Also call the default handler for console output in debug builds
#ifdef __WXDEBUG__
    // Only log to console, don't call parent (which could cause recursion)
    if (wxLog::GetActiveTarget() != this) {
        fprintf(stderr, "[%s] %s\n", levelStr.c_str(), msg.c_str());
    }
#endif
}

// CustomAssertHandler Implementation
void CustomAssertHandler::HandleAssertion(const wxString& file, int line, const wxString& func,
                                         const wxString& cond, const wxString& msg)
{
    if (!ErrorHandler::Instance().m_assertionHandlingEnabled) {
        return;
    }
    
    wxString title = "wxWidgets Assertion Failed";
    wxString message = wxString::Format("Assertion '%s' failed", cond);
    
    if (!msg.IsEmpty()) {
        message += ":\n" + msg;
    }
    
    wxString details = wxString::Format("File: %s\nLine: %d\nFunction: %s\n\nCondition: %s",
                                       file, line, func, cond);
    
    // Report as warning instead of error to avoid being too alarming
    ErrorHandler::Instance().ShowErrorDialog(title, message, details, wxICON_WARNING);
}

// ErrorDialog Implementation
ErrorDialog::ErrorDialog(wxWindow* parent, const wxString& title, const wxString& message,
                        const wxString& details, int iconType)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    CreateControls(message, details, iconType);
    
    // Prepare full error text for clipboard
    m_fullErrorText = wxString::Format("Title: %s\n\nMessage:\n%s", title, message);
    if (!details.IsEmpty()) {
        m_fullErrorText += "\n\nDetails:\n" + details;
    }
    m_fullErrorText += "\n\nTimestamp: " + wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S");
}

void ErrorDialog::CreateControls(const wxString& message, const wxString& details, int iconType)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Header with icon and message
    wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Icon
    wxArtID artId = wxART_INFORMATION;
    if (iconType == wxICON_ERROR) artId = wxART_ERROR;
    else if (iconType == wxICON_WARNING) artId = wxART_WARNING;
    else if (iconType == wxICON_QUESTION) artId = wxART_QUESTION;
    
    wxStaticBitmap* icon = new wxStaticBitmap(this, wxID_ANY, 
        wxArtProvider::GetBitmap(artId, wxART_MESSAGE_BOX));
    headerSizer->Add(icon, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    
    // Message text
    wxStaticText* msgText = new wxStaticText(this, wxID_ANY, message);
    msgText->Wrap(400);
    headerSizer->Add(msgText, 1, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    
    mainSizer->Add(headerSizer, 0, wxEXPAND);
    
    // Details section (if provided)
    if (!details.IsEmpty()) {
        wxStaticText* detailsLabel = new wxStaticText(this, wxID_ANY, "Details:");
        detailsLabel->SetFont(detailsLabel->GetFont().Bold());
        mainSizer->Add(detailsLabel, 0, wxALL, 5);
        
        m_detailsText = new wxTextCtrl(this, ID_ERROR_DETAILS, details,
                                      wxDefaultPosition, wxSize(500, 150),
                                      wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
        m_detailsText->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        mainSizer->Add(m_detailsText, 1, wxALL | wxEXPAND, 5);
    }
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxButton* copyBtn = new wxButton(this, ID_COPY_TO_CLIPBOARD, "Copy && Close");
    // Make the copy button wider to accommodate larger confirmation text
    copyBtn->SetMinSize(wxSize(240, -1));
    
    wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
    okBtn->SetDefault();
    
    buttonSizer->Add(copyBtn, 0, wxRIGHT, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(okBtn, 0);
    
    mainSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 10);
    
    SetSizer(mainSizer);
    
    // Size dialog appropriately
    if (details.IsEmpty()) {
        SetSize(450, 200);
    } else {
        SetSize(550, 400);
    }
    
    Center();
}

void ErrorDialog::OnCopyToClipboard(wxCommandEvent& WXUNUSED(event))
{
    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(m_fullErrorText));
        wxTheClipboard->Close();
        
        // Change button text to show confirmation and prepare to close
        wxButton* copyBtn = wxDynamicCast(FindWindow(ID_COPY_TO_CLIPBOARD), wxButton);
        if (copyBtn) {
            copyBtn->SetLabel("Copied, closing...");
            copyBtn->SetBackgroundColour(wxColour(40, 167, 69)); // Green background
            copyBtn->SetForegroundColour(wxColour(255, 255, 255)); // Explicit white
            
            // Make text bold and larger for better readability
            wxFont font = copyBtn->GetFont();
            font.MakeBold();
            font.SetPointSize(font.GetPointSize() + 2); // Increase size by 2 points
            copyBtn->SetFont(font);
            
            copyBtn->Enable(false); // Disable to prevent multiple clicks
            copyBtn->Refresh();
            
            // Close dialog after 1 second
            wxTimer* timer = new wxTimer();
            timer->Bind(wxEVT_TIMER, [this, timer](wxTimerEvent&) {
                delete timer;
                EndModal(wxID_OK);
            });
            timer->StartOnce(1000); // 1 second
        }
    } else {
        // If clipboard failed, just close normally
        EndModal(wxID_OK);
    }
}

void ErrorDialog::OnClose(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_OK);
}
