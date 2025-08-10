/**
 * gui/SettingsPanel.cpp
 * Settings Panel stub implementation
 */

#include "SettingsPanel.h"
#include "core/StateManager.h"

// Stub implementation
SettingsPanel::SettingsPanel(wxWindow* parent)
    : wxPanel(parent), m_stateManager(StateManager::getInstance())
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* text = new wxStaticText(this, wxID_ANY, "Settings Panel\n(To be implemented)");
    sizer->Add(text, 1, wxALL | wxEXPAND, 10);
    SetSizer(sizer);
}
