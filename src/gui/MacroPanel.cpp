/**
 * gui/MacroPanel.cpp
 * Macro Panel stub implementation
 */

#include "MacroPanel.h"

// Stub implementation
MacroPanel::MacroPanel(wxWindow* parent) : wxPanel(parent) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(this, wxID_ANY, "Macro Panel\n(To be implemented)"), 1, wxALL | wxEXPAND, 10);
    SetSizer(sizer);
}
