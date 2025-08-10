/**
 * core/UpdateChecker.h
 * Handles application update checking and usage analytics
 * Sends anonymous usage data to GitHub Pages analytics endpoint
 */

#pragma once

#include <wx/wx.h>
#include <wx/url.h>
#include <wx/stream.h>
#include <wx/protocol/http.h>
#include <wx/thread.h>
#include <string>
#include <functional>

struct UpdateInfo {
    bool updateAvailable = false;
    wxString latestVersion;
    wxString currentVersion;
    wxString downloadUrl;
    wxString releaseNotes;
    wxString error;
};

struct AnalyticsData {
    wxString version;
    wxString operatingSystem;
    wxString platform;
    wxString locale;
    wxString timestamp;
    bool isFirstRun = false;
};

class UpdateChecker : public wxThread {
public:
    UpdateChecker();
    ~UpdateChecker();
    
    // Static methods for easy access
    static void CheckForUpdatesAsync(std::function<void(const UpdateInfo&)> callback = nullptr);
    static void SendAnalyticsAsync(const AnalyticsData& data = {});
    static void TrackAppStart();
    
    // Synchronous methods (use with caution - may block)
    static UpdateInfo CheckForUpdatesSync();
    static bool SendAnalyticsSync(const AnalyticsData& data = {});
    
    // Configuration
    static void SetAnalyticsEndpoint(const wxString& url);
    static void SetUpdateEndpoint(const wxString& url);
    static void EnableAnalytics(bool enable = true);
    
    // Utility methods
    static AnalyticsData CreateAnalyticsData();
    static wxString GetCurrentVersion();
    static wxString GetOperatingSystemInfo();
    static wxString GetPlatformInfo();
    
private:
    // Thread implementation
    virtual wxThread::ExitCode Entry() override;
    
    // HTTP operations
    bool PerformHttpRequest(const wxString& url, const wxString& data = wxEmptyString);
    wxString MakeHttpGetRequest(const wxString& url);
    bool MakeHttpPostRequest(const wxString& url, const wxString& data);
    
    // Update checking
    UpdateInfo ParseUpdateResponse(const wxString& response);
    
    // Analytics
    wxString CreateAnalyticsPayload(const AnalyticsData& data);
    
    // Configuration
    static wxString s_analyticsEndpoint;
    static wxString s_updateEndpoint;
    static bool s_analyticsEnabled;
    
    // Callback for async operations
    std::function<void(const UpdateInfo&)> m_updateCallback;
    AnalyticsData m_analyticsData;
    bool m_isUpdateCheck;
    
    // HTTP client
    wxHTTP* m_http;
};

// Helper class for managing update checking in the main application
class UpdateManager {
public:
    static void Initialize();
    static void CheckForUpdatesOnStartup();
    static void TrackApplicationStart();
    static void ShowUpdateDialog(wxWindow* parent, const UpdateInfo& info);
    
private:
    static bool s_initialized;
    static void OnUpdateCheckComplete(const UpdateInfo& info);
};
