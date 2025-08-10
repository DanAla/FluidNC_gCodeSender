/**
 * gui/GCodePanel.cpp
 * GCode Panel stub implementation
 */

#include "GCodePanel.h"

// Stub implementation
GCodePanel::GCodePanel(wxWindow* parent) : wxPanel(parent) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(this, wxID_ANY, "GCode Panel\n(To be implemented)"), 1, wxALL | wxEXPAND, 10);
    SetSizer(sizer);
}
