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
#include <wx/stackwalk.h>
#include <wx/msw/debughlp.h>
#include <wx/msw/private.h>
#include <wx/app.h>
#include <sstream>
#include <exception>
#include <csignal>

#ifdef __WXMSW__
#include <windows.h>
#include <tlhelp32.h>
#include <cstring>
#endif

// Dummy stack walker class
// Global terminate handler
void terminateHandler() {
    try {
        std::string error = "Application is terminating due to uncaught exception";
        std::string stackTrace = "No stack trace available";

        // First try to log
        try {
            LOG_ERROR(error + "\n" + stackTrace);
        } catch (...) {
            // Failed to log, try to write to stderr
            fprintf(stderr, "%s\n%s\n", error.c_str(), stackTrace.c_str());
        }

        // Try to show error dialog
        try {
            wxMessageBox(
                error + "\n\n" + stackTrace,
                "Fatal Error",
                wxOK | wxICON_ERROR
            );
        } catch (...) {
            // If dialog fails, at least we logged it
        }
    } catch (...) {
        // Last resort
        fprintf(stderr, "Fatal error: Application is terminating\n");
    }
    abort(); // Terminate the program
}

class StackTrace {
public:
    void Walk(size_t skip = 0) {}
    std::string GetStackTrace() const { return "Stack trace not available."; }
};

// Global exception handler helper
void HandleUnhandledException(const std::string& error, const std::string& stackTrace) {
    try {
        LOG_ERROR("Unhandled exception: " + error + "\n" + stackTrace);
        
        try {
            ErrorHandler::Instance().ReportError(
                "Unhandled Exception",
                "An unexpected error has occurred",
                "Error: " + error + "\n\n"
                "Stack trace:\n" + stackTrace + "\n\n"
                "Please report this error to the developers."
            );
        } catch (...) {
            // Fallback to basic message box
            try {
                wxMessageBox(
                    "An unexpected error has occurred:\n\n" + 
                    error + "\n\n" +
                    "Stack trace:\n" + stackTrace + "\n\n" +
                    "Please report this error to the developers.",
                    "Unhandled Exception",
                    wxOK | wxICON_ERROR
                );
            } catch (...) {
                // Last resort - output to stderr
                fprintf(stderr, "FATAL ERROR: %s\n%s\n", error.c_str(), stackTrace.c_str());
            }
        }
    } catch (...) {
        // Last resort if even error handling fails
        wxMessageBox(
            "A critical error has occurred and could not be properly reported.\n\n"
            "Original error: " + error,
            "Critical Error",
            wxOK | wxICON_ERROR
        );
    }
}

bool FluidNCApp::OnInit()
{
    // Set global terminate handler
    std::set_terminate(terminateHandler);
    // Enable global exception handling
    wxDisableAsserts();  // Disable wx asserts which can interfere with our error handling
    SetExitOnFrameDelete(true);  // Let wxWidgets manage frame lifetime
    
    // Initialize error handling FIRST - before anything else
    ErrorHandler::Instance().Initialize();
    
    // Check for single instance BEFORE creating UI
    m_singleInstanceChecker = new wxSingleInstanceChecker(wxT("FluidNC_gCodeSender"));
    
    if (IsAnotherInstanceRunning()) {
        LOG_INFO("Another instance is already running. Attempting to bring it to front...");
        
        if (BringExistingInstanceToFront()) {
            LOG_INFO("Successfully brought existing instance to front. Exiting this instance.");
        } else {
            LOG_INFO("Failed to bring existing instance to front, but will still exit to prevent conflicts.");
        }
        
        // Clean up and exit
        delete m_singleInstanceChecker;
        m_singleInstanceChecker = nullptr;
        return false; // Exit this instance
    }
    
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
    // Clean up single instance checker
    if (m_singleInstanceChecker) {
        delete m_singleInstanceChecker;
        m_singleInstanceChecker = nullptr;
    }
    
    return wxApp::OnExit();
}

bool FluidNCApp::IsAnotherInstanceRunning()
{
    if (!m_singleInstanceChecker) {
        return false;
    }
    
    return m_singleInstanceChecker->IsAnotherRunning();
}

void FluidNCApp::OnUnhandledException() {
    try {
        // Get current exception info
        std::string error = "Unknown error";
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception& e) {
            error = e.what();
        } catch (...) {
            error = "Unknown exception type";
        }
        
        // Get stack trace
        StackTrace stackWalker;
        stackWalker.Walk(2); // Skip App's handler frame
        std::string stackTrace = stackWalker.GetStackTrace();
        
        HandleUnhandledException(error, stackTrace);
        
    } catch (...) {
        // Last resort
        wxMessageBox(
            "A critical error has occurred and could not be handled properly.",
            "Critical Error",
            wxOK | wxICON_ERROR
        );
    }
}

bool FluidNCApp::BringExistingInstanceToFront()
{
#ifdef __WXMSW__
    // On Windows, find the existing window and bring it to front
    // First try to find window by looking for the title pattern
    HWND hwnd = nullptr;
    
    // Enumerate all windows and find one with our app name in the title
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        HWND* resultHwnd = reinterpret_cast<HWND*>(lParam);
        
        WCHAR windowTitle[512];
        if (GetWindowText(hwnd, windowTitle, sizeof(windowTitle) / sizeof(WCHAR)) > 0) {
            std::wstring title(windowTitle);
            // Look for our app name in the window title
            if (title.find(L"FluidNC gCode Sender") != std::wstring::npos) {
                // Make sure it's a visible main window
                if (IsWindowVisible(hwnd) && GetParent(hwnd) == nullptr) {
                    *resultHwnd = hwnd;
                    return FALSE; // Stop enumeration
                }
            }
        }
        return TRUE; // Continue enumeration
    }, reinterpret_cast<LPARAM>(&hwnd));
    
    if (hwnd) {
        // If window is minimized, restore it
        if (IsIconic(hwnd)) {
            ShowWindow(hwnd, SW_RESTORE);
        }
        
        // Bring window to front
        SetForegroundWindow(hwnd);
        
        // Flash the window to get user attention
        FlashWindow(hwnd, TRUE);
        
        return true;
    }
    
    // Alternative approach: try to find by process name
    PROCESSENTRY32 pe32;
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (wcsstr(pe32.szExeFile, L"FluidNC_gCodeSender.exe") != NULL) {
                // Found the process, try to find its main window
                DWORD processId = pe32.th32ProcessID;
                if (processId != GetCurrentProcessId()) {
                    // Enumerate windows to find the main window of this process
                    struct EnumData {
                        DWORD processId;
                        HWND hwnd;
                    } enumData = { processId, NULL };
                    
                    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                        EnumData* data = reinterpret_cast<EnumData*>(lParam);
                        DWORD windowProcessId;
                        GetWindowThreadProcessId(hwnd, &windowProcessId);
                        
                        if (windowProcessId == data->processId && IsWindowVisible(hwnd)) {
                            WCHAR className[256];
                            ::GetClassNameW(hwnd, className, sizeof(className) / sizeof(WCHAR));
                            
                            // Check if this looks like a main window (has a title bar)
                            if (GetWindowTextLength(hwnd) > 0) {
                                data->hwnd = hwnd;
                                return FALSE; // Stop enumeration
                            }
                        }
                        return TRUE; // Continue enumeration
                    }, reinterpret_cast<LPARAM>(&enumData));
                    
                    if (enumData.hwnd) {
                        if (IsIconic(enumData.hwnd)) {
                            ShowWindow(enumData.hwnd, SW_RESTORE);
                        }
                        SetForegroundWindow(enumData.hwnd);
                        FlashWindow(enumData.hwnd, TRUE);
                        CloseHandle(hProcessSnap);
                        return true;
                    }
                }
                break;
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    
    CloseHandle(hProcessSnap);
    return false;
    
#else
    // On non-Windows platforms, we can't easily bring window to front
    // The single instance checker will prevent the second instance from starting
    return false;
#endif
}
