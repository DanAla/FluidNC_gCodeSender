/**
 * FluidNC_gCodeSender/App.h
 * Main application class for FluidNC_gCodeSender
 */

#pragma once

#include <wx/wx.h>
#include <wx/snglinst.h>

class MainFrame;

class FluidNCApp : public wxApp
{
public:
    virtual bool OnInit() override;
    virtual int OnExit() override;
    virtual void OnUnhandledException() override;

private:
    MainFrame* m_mainFrame;
    wxSingleInstanceChecker* m_singleInstanceChecker;
    
    // Helper methods for single instance handling
    bool IsAnotherInstanceRunning();
    bool BringExistingInstanceToFront();
};
