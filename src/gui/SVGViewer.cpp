/**
 * gui/SVGViewer.cpp
 * SVG Viewer stub implementation
 */

#include "SVGViewer.h"

// Stub implementation
SVGViewer::SVGViewer(wxWindow* parent)
    : wxPanel(parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* text = new wxStaticText(this, wxID_ANY, "SVG Viewer\n(To be implemented)");
    sizer->Add(text, 1, wxALL | wxEXPAND, 10);
    SetSizer(sizer);
}
