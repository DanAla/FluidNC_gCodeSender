/**
 * core/ErrorHandler.h
 * Comprehensive error handling system for wxWidgets applications
 * Catches assertions, exceptions, and other errors gracefully
 */

#pragma once

#include <wx/wx.h>
#include <wx/log.h>
#include <string>
#include <vector>
#include <memory>

/**
 * Custom error handler that catches wxWidgets assertions and other errors
 * Displays errors in a copyable, user-friendly dialog instead of crashing
 */
class ErrorHandler
{
public:
    static ErrorHandler& Instance();
    
    // Initialize error handling
    void Initialize();
    
    // Error reporting methods
    void ReportError(const wxString& title, const wxString& message, const wxString& details = wxEmptyString);
    void ReportWarning(const wxString& title, const wxString& message, const wxString& details = wxEmptyString);
    void ReportInfo(const wxString& title, const wxString& message, const wxString& details = wxEmptyString);
    
    // Enable/disable assertion handling
    void EnableAssertionHandling(bool enable = true);
    
    // Get recent errors for debugging
    std::vector<wxString> GetRecentErrors() const;
    void ClearErrors();

private:
    ErrorHandler() = default;
    ~ErrorHandler() = default;
    
    // Singleton - no copying
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;
    
    // Internal methods
    void ShowErrorDialog(const wxString& title, const wxString& message, 
                        const wxString& details, int iconType);
    
    // Error storage
    std::vector<wxString> m_recentErrors;
    static const size_t MAX_STORED_ERRORS = 50;
    
    bool m_assertionHandlingEnabled = true;
    
    // Friend classes need access to private methods
    friend class CustomLogTarget;
    friend class CustomAssertHandler;
};

/**
 * Custom log target that redirects wxWidgets log messages to our error handler
 */
class CustomLogTarget : public wxLog
{
public:
    CustomLogTarget();
    virtual ~CustomLogTarget();

protected:
    virtual void DoLogRecord(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info) override;
};

/**
 * Custom assertion handler that prevents crashes and shows user-friendly dialogs
 */
class CustomAssertHandler
{
public:
    static void HandleAssertion(const wxString& file, int line, const wxString& func,
                               const wxString& cond, const wxString& msg);
};

/**
 * Error dialog that shows detailed error information with copy functionality
 */
class ErrorDialog : public wxDialog
{
public:
    ErrorDialog(wxWindow* parent, const wxString& title, const wxString& message,
               const wxString& details, int iconType);

private:
    void CreateControls(const wxString& message, const wxString& details, int iconType);
    void OnCopyToClipboard(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    
    wxTextCtrl* m_detailsText;
    wxString m_fullErrorText;
    
    wxDECLARE_EVENT_TABLE();
};
