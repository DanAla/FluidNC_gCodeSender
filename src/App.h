/**
 * FluidNC_gCodeSender/App.h
 * Main application class for FluidNC_gCodeSender
 */

#pragma once

#include <wx/wx.h>

class MainFrame;

class FluidNCApp : public wxApp
{
public:
    virtual bool OnInit() override;
    virtual int OnExit() override;

private:
    MainFrame* m_mainFrame;
};
