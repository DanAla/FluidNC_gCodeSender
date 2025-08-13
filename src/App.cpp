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
#include <tlhelp32.h>
#include <string>
#endif

bool FluidNCApp::OnInit()
{
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
