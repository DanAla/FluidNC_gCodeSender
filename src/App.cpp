/**
 * FluidNC_gCodeSender/App.cpp
 * Main application class implementation
 */

#include "App.h"
#include "gui/MainFrame.h"
#include "gui/WelcomeDialog.h"
#include "core/SimpleLogger.h"
#include <wx/wx.h>

bool FluidNCApp::OnInit()
{
    LOG_INFO("=== FluidNC gCode Sender Application Starting ===");
    
    try {
        LOG_INFO("Creating MainFrame...");
        // Create the main frame
        m_mainFrame = new MainFrame();
        
        LOG_INFO("Showing MainFrame...");
        // Show the main window
        m_mainFrame->Show(true);
        m_mainFrame->Center();
        m_mainFrame->Raise();
        m_mainFrame->SetFocus();
        
        LOG_INFO("MainFrame displayed successfully");
        
        // Show welcome dialog if user hasn't disabled it
        WelcomeDialog::ShowWelcomeIfNeeded(m_mainFrame);
        
        LOG_INFO("Application initialization completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        wxMessageBox(wxString::Format("Failed to start application: %s", e.what()),
                     "Startup Error", wxOK | wxICON_ERROR);
        return false;
    } catch (...) {
        wxMessageBox("Unknown error occurred during startup",
                     "Startup Error", wxOK | wxICON_ERROR);
        return false;
    }
}

int FluidNCApp::OnExit()
{
    return wxApp::OnExit();
}
