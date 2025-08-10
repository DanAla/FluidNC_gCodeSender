/**
 * gui/AboutDialog.h
 * Custom About dialog with clickable links and enhanced information
 */

#pragma once

#include <wx/wx.h>
#include <wx/hyperlink.h>
#include <wx/stattext.h>
#include <wx/sizer.h>

class AboutDialog : public wxDialog
{
public:
    AboutDialog(wxWindow* parent);
    virtual ~AboutDialog();

private:
    void CreateControls();
    void DoLayout();
    void LoadBuildInformation();
    void UpdateBuildInfoDisplay();
    
    // Event handlers
    void OnClose(wxCloseEvent& event);
    void OnOK(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);
    
    // Controls
    wxStaticText* m_titleText;
    wxStaticText* m_descriptionText;
    wxStaticText* m_featuresText;
    wxHyperlinkCtrl* m_repositoryLink;
    wxHyperlinkCtrl* m_issuesLink;
    wxStaticText* m_buildInfoText;
    wxButton* m_okButton;
    
    // Loading state
    wxTimer* m_loadTimer;
    bool m_buildInfoLoaded;
    std::string m_buildInfoString;
    
    DECLARE_EVENT_TABLE()
};
