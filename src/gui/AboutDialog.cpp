/**
 * gui/AboutDialog.cpp
 * Custom About dialog implementation
 */

#include "AboutDialog.h"
#include "core/Version.h"
#include <wx/font.h>
#include <wx/timer.h>
#include <thread>

// Event table
wxBEGIN_EVENT_TABLE(AboutDialog, wxDialog)
    EVT_BUTTON(wxID_OK, AboutDialog::OnOK)
    EVT_CLOSE(AboutDialog::OnClose)
    EVT_TIMER(wxID_ANY, AboutDialog::OnTimer)
wxEND_EVENT_TABLE()

AboutDialog::AboutDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxString::Format("About %s", FluidNC::Version::APP_NAME),
               wxDefaultPosition, wxDefaultSize, 
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_loadTimer(nullptr),
      m_buildInfoLoaded(false)
{
    // Initialize loading state
    m_buildInfoLoaded = false;
    m_buildInfoString = "";
    
    CreateControls();
    DoLayout();
    
    // Start loading build information
    LoadBuildInformation();
    
    // Set appropriate size to fit all content
    SetSize(600, 550);
    
    // Center the dialog
    Center();
    
    // Set minimum size
    SetMinSize(wxSize(550, 500));
}

AboutDialog::~AboutDialog()
{
    if (m_loadTimer) {
        m_loadTimer->Stop();
        delete m_loadTimer;
    }
}

void AboutDialog::CreateControls()
{
    // Title text
    m_titleText = new wxStaticText(this, wxID_ANY, FluidNC::Version::GetFullVersionString());
    wxFont titleFont = m_titleText->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 4);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_titleText->SetFont(titleFont);
    
    // Description text
    m_descriptionText = new wxStaticText(this, wxID_ANY,
        "Professional CNC Control Application\n"
        "Built with C++ and wxWidgets");
    
    // Features text (use centralized function to avoid duplication)
    m_featuresText = new wxStaticText(this, wxID_ANY, 
        wxString::FromUTF8(FluidNC::Version::GetFeaturesString()));
    
    // Clickable links
    m_repositoryLink = new wxHyperlinkCtrl(this, wxID_ANY,
        "GitHub Repository",
        FluidNC::Version::REPOSITORY_URL);
    
    m_issuesLink = new wxHyperlinkCtrl(this, wxID_ANY,
        "Report Issues",
        FluidNC::Version::ISSUES_URL);
    
    // Build information (monospaced font for alignment)
    // Initially empty, will be populated by LoadBuildInformation
    m_buildInfoText = new wxStaticText(this, wxID_ANY, "");
    wxFont monoFont(wxFontInfo().Family(wxFONTFAMILY_TELETYPE));
    m_buildInfoText->SetFont(monoFont);
    
    // OK button
    m_okButton = new wxButton(this, wxID_OK, "&OK");
    m_okButton->SetDefault();
}

void AboutDialog::DoLayout()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    mainSizer->Add(m_titleText, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);
    
    // Description
    mainSizer->Add(m_descriptionText, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, 10);
    
    // Features
    mainSizer->Add(m_featuresText, 0, wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    // Links section
    wxStaticBoxSizer* linksSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Links");
    linksSizer->Add(m_repositoryLink, 0, wxALL, 5);
    linksSizer->Add(m_issuesLink, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);
    mainSizer->Add(linksSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);
    
    // Build information section
    wxStaticBoxSizer* buildSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Build Information");
    buildSizer->Add(m_buildInfoText, 1, wxALL | wxEXPAND, 5);
    mainSizer->Add(buildSizer, 1, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);
    
    // Button
    mainSizer->Add(m_okButton, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);
    
    SetSizer(mainSizer);
    Layout();
}

void AboutDialog::OnClose(wxCloseEvent& event)
{
    EndModal(wxID_CANCEL);
}

void AboutDialog::OnOK(wxCommandEvent& event)
{
    EndModal(wxID_OK);
}

void AboutDialog::LoadBuildInformation()
{
    // Initially show loading message
    m_buildInfoText->SetLabel("Loading build information...\n\nOne moment please...");
    
    // Start timer to simulate loading and then update with real info
    m_loadTimer = new wxTimer(this);
    m_loadTimer->Start(500, wxTIMER_ONE_SHOT);  // Wait 500ms then update
}

void AboutDialog::OnTimer(wxTimerEvent& event)
{
    // Load the actual build information
    UpdateBuildInfoDisplay();
}

void AboutDialog::UpdateBuildInfoDisplay()
{
    // Get the complete build information
    m_buildInfoString = FluidNC::Version::GetBuildInfoString();
    m_buildInfoLoaded = true;
    
    // Update the display
    m_buildInfoText->SetLabel(wxString::FromUTF8(m_buildInfoString));
    
    // Force layout update to accommodate the new text
    Layout();
    Fit();
    
    // Ensure minimum size is maintained
    wxSize currentSize = GetSize();
    wxSize minSize = GetMinSize();
    
    if (currentSize.x < minSize.x || currentSize.y < minSize.y) {
        SetSize(wxSize(std::max(currentSize.x, minSize.x), 
                      std::max(currentSize.y, minSize.y)));
    }
    
    // Re-center if needed
    Center();
}
