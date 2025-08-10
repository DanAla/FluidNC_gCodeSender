/**
 * gui/TelnetSetupPanel.cpp
 * Telnet Setup Panel stub implementation
 */

#include "TelnetSetupPanel.h"

// Stub implementation
TelnetSetupPanel::TelnetSetupPanel(wxWindow* parent)
    : wxPanel(parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* text = new wxStaticText(this, wxID_ANY, "Telnet Setup Panel\n(To be implemented)");
    sizer->Add(text, 1, wxALL | wxEXPAND, 10);
    SetSizer(sizer);
}
