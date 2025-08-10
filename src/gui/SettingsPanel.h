/**
 * gui/SettingsPanel.h
 * Settings Panel stub header
 */

#pragma once
#include <wx/wx.h>

class StateManager;

class SettingsPanel : public wxPanel
{
public:
    SettingsPanel(wxWindow* parent);
    
private:
    StateManager& m_stateManager;
};
