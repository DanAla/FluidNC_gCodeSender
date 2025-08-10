/**
 * gui/WelcomeDialog.cpp
 * Welcome dialog with "Don't show this again" option
 */

#include "WelcomeDialog.h"
#include <wx/config.h>
#include <wx/stdpaths.h>

// Event table
wxBEGIN_EVENT_TABLE(WelcomeDialog, wxDialog)
    EVT_BUTTON(wxID_OK, WelcomeDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, WelcomeDialog::OnCancel)
wxEND_EVENT_TABLE()

WelcomeDialog::WelcomeDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Welcome to FluidNC gCode Sender", 
               wxDefaultPosition, wxDefaultSize, 
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    CreateControls();
    Centre();
}

void WelcomeDialog::CreateControls()
{
    // Main sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Welcome icon and title
    wxStaticText* titleText = new wxStaticText(this, wxID_ANY, 
        "FluidNC gCode Sender");
    wxFont titleFont = titleText->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 4);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleText->SetFont(titleFont);
    
    mainSizer->Add(titleText, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 15);
    
    // Welcome message
    wxStaticText* messageText = new wxStaticText(this, wxID_ANY,
        "Welcome to the professional CNC control application!\n\n"
        "FluidNC gCode Sender provides comprehensive control for your CNC machines\n"
        "with support for multiple connection types including Telnet, USB, and UART.\n\n"
        "Features:\n"
        "• Professional multi-machine management\n"
        "• Real-time machine status monitoring\n"
        "• Advanced jogging controls\n"
        "• G-code editing and visualization\n"
        "• Macro system for automation\n"
        "• Flexible docking interface\n\n"
        "Get started by connecting to your FluidNC machine through the\n"
        "Machine menu or toolbar buttons.");
    
    mainSizer->Add(messageText, 1, wxALL | wxEXPAND, 15);
    
    // Don't show again checkbox
    m_dontShowAgainCheckbox = new wxCheckBox(this, wxID_ANY, 
        "Don't show this welcome message again");
    mainSizer->Add(m_dontShowAgainCheckbox, 0, wxALL, 15);
    
    // Button sizer
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxButton* okButton = new wxButton(this, wxID_OK, "Get Started");
    okButton->SetDefault();
    buttonSizer->Add(okButton, 0, wxRIGHT, 10);
    
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Close");
    buttonSizer->Add(cancelButton, 0, 0, 0);
    
    mainSizer->Add(buttonSizer, 0, wxALL | wxALIGN_CENTER, 15);
    
    SetSizer(mainSizer);
    
    // Size the dialog appropriately
    SetInitialSize(wxSize(500, 400));
    SetMinSize(wxSize(400, 300));
}

void WelcomeDialog::OnOK(wxCommandEvent& WXUNUSED(event))
{
    // Save the "don't show again" preference if checked
    if (m_dontShowAgainCheckbox->IsChecked()) {
        wxConfig config("FluidNC_gCodeSender");
        config.Write("ShowWelcomeDialog", false);
        config.Flush();
    }
    
    EndModal(wxID_OK);
}

void WelcomeDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    // Save the "don't show again" preference if checked
    if (m_dontShowAgainCheckbox->IsChecked()) {
        wxConfig config("FluidNC_gCodeSender");
        config.Write("ShowWelcomeDialog", false);
        config.Flush();
    }
    
    EndModal(wxID_CANCEL);
}

// Static method to check if welcome should be shown
bool WelcomeDialog::ShouldShowWelcome()
{
    wxConfig config("FluidNC_gCodeSender");
    bool showWelcome = true; // Default to showing welcome
    config.Read("ShowWelcomeDialog", &showWelcome, true);
    return showWelcome;
}

// Static method to show welcome if needed
void WelcomeDialog::ShowWelcomeIfNeeded(wxWindow* parent)
{
    if (ShouldShowWelcome()) {
        WelcomeDialog dialog(parent);
        dialog.ShowModal();
    }
}
