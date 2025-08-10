/**
 * gui/WelcomeDialog.h
 * Welcome dialog with "Don't show this again" option
 */

#pragma once

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/sizer.h>

class WelcomeDialog : public wxDialog
{
public:
    WelcomeDialog(wxWindow* parent);
    
    // Check if dialog should be shown based on user preference
    static bool ShouldShowWelcome();
    
    // Show the welcome dialog if needed
    static void ShowWelcomeIfNeeded(wxWindow* parent);

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void CreateControls();
    
    wxCheckBox* m_dontShowAgainCheckbox;
    
    wxDECLARE_EVENT_TABLE();
};
