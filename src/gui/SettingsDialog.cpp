/**
 * gui/SettingsDialog.cpp
 * Settings Dialog stub implementation
 */

#include "SettingsDialog.h"
#include "core/StateManager.h"
#include "core/ConnectionManager.h"

wxBEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, SettingsDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, SettingsDialog::OnCancel)
    // More event bindings will be added when implementing full functionality
wxEND_EVENT_TABLE()

// Stub implementation
SettingsDialog::SettingsDialog(wxWindow* parent, StateManager& stateManager, ConnectionManager& connectionManager)
    : wxDialog(parent, wxID_ANY, "Settings", wxDefaultPosition, wxSize(600, 400)),
      m_stateManager(stateManager), m_connectionManager(connectionManager)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* text = new wxStaticText(this, wxID_ANY, "Settings Dialog\n(To be implemented)");
    sizer->Add(text, 1, wxALL | wxEXPAND, 20);
    
    // Add OK/Cancel buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxALL, 5);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxALL, 5);
    sizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);
    
    SetSizer(sizer);
}

SettingsDialog::~SettingsDialog() {
    // Cleanup
}

void SettingsDialog::ShowTab(int tabIndex) {
    // TODO: Implement tab switching
}

void SettingsDialog::OnOK(wxCommandEvent& event) {
    EndModal(wxID_OK);
}

void SettingsDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}
