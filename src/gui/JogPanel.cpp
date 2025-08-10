/**
 * gui/JogPanel.cpp
 * Jog Panel stub implementation
 */

#include "JogPanel.h"
#include "core/StateManager.h"
#include "core/ConnectionManager.h"

wxBEGIN_EVENT_TABLE(JogPanel, wxPanel)
    // Event bindings will be added here when implementing full functionality
wxEND_EVENT_TABLE()

// Stub implementation
JogPanel::JogPanel(wxWindow* parent, ConnectionManager* connectionManager)
    : wxPanel(parent), m_connectionManager(connectionManager), m_stateManager(StateManager::getInstance())
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* text = new wxStaticText(this, wxID_ANY, "Jog Panel\n(To be implemented)");
    sizer->Add(text, 1, wxALL | wxEXPAND, 10);
    SetSizer(sizer);
}
