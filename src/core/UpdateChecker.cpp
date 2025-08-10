/**
 * core/UpdateChecker.cpp
 * Implementation of update checking and usage analytics
 */

#include "UpdateChecker.h"
#include "Version.h"
#include "ErrorHandler.h"
#include "../gui/NotificationSystem.h"
#include <wx/protocol/http.h>
#include <wx/url.h>
#include <wx/stream.h>
#include <wx/platinfo.h>
#include <wx/utils.h>
#include <wx/datetime.h>
#include <wx/msgdlg.h>
#include <wx/hyperlink.h>
#include <wx/regex.h>

// Static member initialization
wxString UpdateChecker::s_analyticsEndpoint = "https://danala.github.io/FluidNC_gCodeSender/";
wxString UpdateChecker::s_updateEndpoint = "https://api.github.com/repos/DanAla/FluidNC_gCodeSender/releases/latest";
bool UpdateChecker::s_analyticsEnabled = true;

bool UpdateManager::s_initialized = false;

// UpdateChecker Implementation
UpdateChecker::UpdateChecker() 
    : wxThread(wxTHREAD_DETACHED), m_http(nullptr), m_isUpdateCheck(false) {
    m_http = new wxHTTP();
    m_http->SetTimeout(10); // 10 second timeout
}

UpdateChecker::~UpdateChecker() {
    if (m_http) {
        delete m_http;
        m_http = nullptr;
    }
}

void UpdateChecker::CheckForUpdatesAsync(std::function<void(const UpdateInfo&)> callback) {
    UpdateChecker* checker = new UpdateChecker();
    checker->m_updateCallback = callback;
    checker->m_isUpdateCheck = true;
    
    if (checker->Create() != wxTHREAD_NO_ERROR || checker->Run() != wxTHREAD_NO_ERROR) {
        delete checker;
        if (callback) {
            UpdateInfo info;
            info.error = "Failed to start update check thread";
            callback(info);
        }
    }
}

void UpdateChecker::SendAnalyticsAsync(const AnalyticsData& data) {
    if (!s_analyticsEnabled) return;
    
    UpdateChecker* checker = new UpdateChecker();
    checker->m_analyticsData = data.version.IsEmpty() ? CreateAnalyticsData() : data;
    checker->m_isUpdateCheck = false;
    
    if (checker->Create() != wxTHREAD_NO_ERROR || checker->Run() != wxTHREAD_NO_ERROR) {
        delete checker;
    }
}

void UpdateChecker::TrackAppStart() {
    if (!s_analyticsEnabled) return;
    
    AnalyticsData data = CreateAnalyticsData();
    data.isFirstRun = false; // Could implement first run detection
    SendAnalyticsAsync(data);
}

UpdateInfo UpdateChecker::CheckForUpdatesSync() {
    UpdateInfo info;
    
    wxHTTP http;
    http.SetTimeout(10);
    
    wxURL url(s_updateEndpoint);
    if (!http.Connect(url.GetServer())) {
        info.error = "Failed to connect to update server";
        return info;
    }
    
    wxInputStream* stream = http.GetInputStream(url.GetPath());
    if (!stream) {
        info.error = "Failed to get update information";
        return info;
    }
    
    wxString response;
    char buffer[1024];
    while (!stream->Eof()) {
        stream->Read(buffer, sizeof(buffer));
        size_t bytesRead = stream->LastRead();
        response += wxString(buffer, wxConvUTF8, bytesRead);
    }
    
    delete stream;
    
    UpdateChecker checker;
    return checker.ParseUpdateResponse(response);
}

bool UpdateChecker::SendAnalyticsSync(const AnalyticsData& data) {
    if (!s_analyticsEnabled) return false;
    
    AnalyticsData analyticsData = data.version.IsEmpty() ? CreateAnalyticsData() : data;
    UpdateChecker checker;
    wxString payload = checker.CreateAnalyticsPayload(analyticsData);
    
    // For GitHub Pages, we'll use a simple GET request with parameters
    // since we can't do POST requests to static sites easily
    wxString url = s_analyticsEndpoint + "track.html?" + payload;
    
    wxHTTP http;
    http.SetTimeout(5); // Shorter timeout for analytics
    
    wxURL trackUrl(url);
    if (!http.Connect(trackUrl.GetServer())) {
        return false;
    }
    
    wxInputStream* stream = http.GetInputStream(trackUrl.GetPath() + "?" + trackUrl.GetQuery());
    if (stream) {
        delete stream;
        return true;
    }
    
    return false;
}

AnalyticsData UpdateChecker::CreateAnalyticsData() {
    AnalyticsData data;
    data.version = GetCurrentVersion();
    data.operatingSystem = GetOperatingSystemInfo();
    data.platform = GetPlatformInfo();
    data.locale = wxLocale::GetSystemLanguage() != wxLANGUAGE_UNKNOWN ? 
                  wxLocale::GetLanguageName(wxLocale::GetSystemLanguage()) : "Unknown";
    data.timestamp = wxDateTime::Now().FormatISOCombined();
    
    return data;
}

wxString UpdateChecker::GetCurrentVersion() {
    return wxString(FluidNC::Version::VERSION_STRING_STR);
}

wxString UpdateChecker::GetOperatingSystemInfo() {
    wxPlatformInfo platform;
    return wxString::Format("%s %d.%d", 
                           platform.GetOperatingSystemIdName(),
                           platform.GetOSMajorVersion(),
                           platform.GetOSMinorVersion());
}

wxString UpdateChecker::GetPlatformInfo() {
    wxPlatformInfo platform;
    return platform.GetBitnessName() + " (" + platform.GetEndiannessName() + ")";
}

void UpdateChecker::SetAnalyticsEndpoint(const wxString& url) {
    s_analyticsEndpoint = url;
}

void UpdateChecker::SetUpdateEndpoint(const wxString& url) {
    s_updateEndpoint = url;
}

void UpdateChecker::EnableAnalytics(bool enable) {
    s_analyticsEnabled = enable;
}

wxThread::ExitCode UpdateChecker::Entry() {
    try {
        if (m_isUpdateCheck) {
            UpdateInfo info = CheckForUpdatesSync();
            if (m_updateCallback) {
                // Call callback in main thread
                wxTheApp->CallAfter([=]() {
                    m_updateCallback(info);
                });
            }
        } else {
            SendAnalyticsSync(m_analyticsData);
        }
    } catch (...) {
        // Silently fail for analytics - don't disturb user experience
    }
    
    return static_cast<wxThread::ExitCode>(0);
}

UpdateInfo UpdateChecker::ParseUpdateResponse(const wxString& response) {
    UpdateInfo info;
    info.currentVersion = GetCurrentVersion();
    
    try {
        // Simple regex-based JSON parsing for GitHub API response
        // Using simpler patterns to avoid escape issues
        wxRegEx tagNameRegex("\"tag_name\":[[:space:]]*\"([^\"]+)\"");
        wxRegEx htmlUrlRegex("\"html_url\":[[:space:]]*\"([^\"]+)\"");
        wxRegEx bodyRegex("\"body\":[[:space:]]*\"([^\"]*)\"");
        
        if (tagNameRegex.Matches(response)) {
            info.latestVersion = tagNameRegex.GetMatch(response, 1);
            
            // Simple version comparison (assumes semantic versioning)
            if (info.latestVersion != info.currentVersion) {
                info.updateAvailable = true;
                
                if (htmlUrlRegex.Matches(response)) {
                    info.downloadUrl = htmlUrlRegex.GetMatch(response, 1);
                }
                
                if (bodyRegex.Matches(response)) {
                    wxString body = bodyRegex.GetMatch(response, 1);
                    // Unescape basic JSON escape sequences
                    body.Replace("\\n", "\n");
                    body.Replace("\\r", "\r");
                    body.Replace("\\t", "\t");
                    body.Replace("\\\\", "\\");
                    body.Replace("\\\"", "\"");
                    info.releaseNotes = body;
                }
            }
        } else {
            info.error = "Invalid response format from GitHub API";
        }
    } catch (...) {
        info.error = "Error processing update information";
    }
    
    return info;
}

wxString UpdateChecker::CreateAnalyticsPayload(const AnalyticsData& data) {
    // Simple URL encoding (basic replacement of special characters)
    auto urlEncode = [](const wxString& str) {
        wxString encoded = str;
        encoded.Replace(" ", "%20");
        encoded.Replace("&", "%26");
        encoded.Replace("=", "%3D");
        encoded.Replace("#", "%23");
        encoded.Replace("?", "%3F");
        return encoded;
    };
    
    // Create URL-encoded parameters for GET request
    wxString payload = wxString::Format(
        "version=%s&os=%s&platform=%s&locale=%s&timestamp=%s&first_run=%s",
        urlEncode(data.version),
        urlEncode(data.operatingSystem),
        urlEncode(data.platform),
        urlEncode(data.locale),
        urlEncode(data.timestamp),
        data.isFirstRun ? "true" : "false"
    );
    
    return payload;
}

// UpdateManager Implementation
void UpdateManager::Initialize() {
    if (s_initialized) return;
    
    // Configure analytics endpoint for GitHub Pages
    UpdateChecker::SetAnalyticsEndpoint("https://danala.github.io/FluidNC_gCodeSender/");
    UpdateChecker::SetUpdateEndpoint("https://api.github.com/repos/DanAla/FluidNC_gCodeSender/releases/latest");
    UpdateChecker::EnableAnalytics(true);
    
    s_initialized = true;
}

void UpdateManager::CheckForUpdatesOnStartup() {
    if (!s_initialized) Initialize();
    
    UpdateChecker::CheckForUpdatesAsync([](const UpdateInfo& info) {
        OnUpdateCheckComplete(info);
    });
}

void UpdateManager::TrackApplicationStart() {
    if (!s_initialized) Initialize();
    
    UpdateChecker::TrackAppStart();
}

void UpdateManager::ShowUpdateDialog(wxWindow* parent, const UpdateInfo& info) {
    if (!info.updateAvailable) return;
    
    // Use notification system instead of modal dialog
    wxString message = wxString::Format(
        "Version %s available (current: %s). Click to download.",
        info.latestVersion,
        info.currentVersion
    );
    
    // Show success notification for available updates with longer duration
    NotificationSystem::Instance().ShowSuccess("Update Available", message, 8000);
    
    // Automatically open browser for convenience
    if (!info.downloadUrl.IsEmpty()) {
        wxLaunchDefaultBrowser(info.downloadUrl);
    }
}

void UpdateManager::OnUpdateCheckComplete(const UpdateInfo& info) {
    if (!info.error.IsEmpty()) {
        // Only show error dialogs for serious connection issues, not parsing issues
        if (info.error.Contains("Failed to connect") || info.error.Contains("Failed to get update")) {
            // Even then, just log silently - update checks should not annoy users
            wxLogMessage("Update check failed: %s", info.error);
        }
        // For parsing errors (like "Invalid response format"), just ignore silently
        // This handles cases where there are no releases yet
        return;
    }
    
    if (info.updateAvailable) {
        // Show update notification in main window if possible
        wxWindow* mainWindow = wxTheApp->GetTopWindow();
        if (mainWindow) {
            ShowUpdateDialog(mainWindow, info);
        }
    }
    // If no update available and no error, just silently succeed
}
