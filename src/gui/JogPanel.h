/**
 * gui/JogPanel.h
 * Machine jogging and homing controls
 */

#pragma once

#include <wx/wx.h>

class ConnectionManager;
class StateManager;

class JogPanel : public wxPanel
{
public:
    JogPanel(wxWindow* parent, ConnectionManager* connectionManager);

private:
    void OnJog(wxCommandEvent& event);
    void OnHome(wxCommandEvent& event);
    void CreateJogControls(wxSizer* sizer);
    void CreateHomingControls(wxSizer* sizer);
    
    ConnectionManager* m_connectionManager;
    StateManager& m_stateManager;
    
    // UI Components
    wxSlider* m_speedSlider;
    
    wxDECLARE_EVENT_TABLE();
};
