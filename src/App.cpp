/**
 * FluidNC_gCodeSender/App.cpp
 * Main application class implementation
 */

#include "App.h"
#include "gui/MainFrame.h"
#include "gui/WelcomeDialog.h"
#include "core/SimpleLogger.h"
#include "core/ErrorHandler.h"
#include "core/UpdateChecker.h"
#include <wx/wx.h>

#ifdef __WXMSW__
#include <windows.h>
#endif

bool FluidNCApp::OnInit()
{
    // Initialize error handling FIRST - before anything else
    ErrorHandler::Instance().Initialize();
    
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
        
        // Force bring window to foreground on Windows
#ifdef __WXMSW__
        // Get the window handle
        HWND hwnd = (HWND)m_mainFrame->GetHandle();
        
        // If window is minimized, restore it
        if (IsIconic(hwnd)) {
            ShowWindow(hwnd, SW_RESTORE);
        }
        
        // Force the window to be foreground with stronger Windows API
        SetForegroundWindow(hwnd);
        
        // Request user attention - flashes in taskbar
        m_mainFrame->RequestUserAttention(wxUSER_ATTENTION_ERROR);
#endif
        
        LOG_INFO("MainFrame displayed successfully");
        
        // Initialize and start update checking & analytics
        LOG_INFO("Initializing update checker and analytics...");
        UpdateManager::Initialize();
        UpdateManager::TrackApplicationStart();
        UpdateManager::CheckForUpdatesOnStartup();
        
        // Show welcome dialog if user hasn't disabled it
        WelcomeDialog::ShowWelcomeIfNeeded(m_mainFrame);
        
        LOG_INFO("Application initialization completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        wxString error = wxString::Format("Failed to start application: %s", e.what());
        ErrorHandler::Instance().ReportError("Startup Error", error);
        return false;
    } catch (...) {
        ErrorHandler::Instance().ReportError("Startup Error", "Unknown error occurred during startup");
        return false;
    }
}

int FluidNCApp::OnExit()
{
    return wxApp::OnExit();
}
