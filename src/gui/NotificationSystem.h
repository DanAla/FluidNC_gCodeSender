/**
 * gui/NotificationSystem.h
 * Modern notification system with floating, auto-dismissing toast notifications
 */

#pragma once

#include <wx/wx.h>
#include <wx/timer.h>
#include <vector>
#include <memory>

class MainFrame;

enum class NotificationType {
    INFO,
    SUCCESS, 
    WARNING,
    ERROR_TYPE  // Renamed to avoid Windows ERROR macro conflict
};

/**
 * Individual notification toast window
 */
class NotificationToast : public wxFrame
{
public:
    NotificationToast(wxWindow* parent, const wxString& title, const wxString& message, 
                      NotificationType type, int duration = 5000);
    ~NotificationToast();

    void StartAutoHide();
    void PauseAutoHide();
    void ResumeAutoHide();
    void ForceClose();
    
    // Position management
    void SetPosition(const wxPoint& pos);
    wxSize GetToastSize() const;
    
    // Animation support
    void AnimateIn();
    void AnimateOut();
    
private:
    void OnTimer(wxTimerEvent& event);
    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnCloseButton(wxCommandEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    
    void CreateControls();
    wxColour GetBackgroundColor() const;
    wxColour GetBorderColor() const;
    wxColour GetTextColor() const;
    wxBitmap GetIcon() const;
    
    wxString m_title;
    wxString m_message;
    NotificationType m_type;
    int m_duration;
    
    wxTimer m_autoHideTimer;
    wxTimer m_animationTimer;
    
    // UI elements
    wxPanel* m_panel;
    wxStaticBitmap* m_iconBitmap;
    wxStaticText* m_titleText;
    wxStaticText* m_messageText;
    wxButton* m_closeButton;
    
    // Animation state
    bool m_isAnimating;
    bool m_isPaused;
    int m_animationStep;
    int m_targetAlpha;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * Notification manager that handles stacking and positioning of toasts
 */
class NotificationSystem
{
public:
    static NotificationSystem& Instance();
    
    // Main notification methods
    void ShowInfo(const wxString& title, const wxString& message, int duration = 5000);
    void ShowSuccess(const wxString& title, const wxString& message, int duration = 4000);
    void ShowWarning(const wxString& title, const wxString& message, int duration = 6000);
    void ShowError(const wxString& title, const wxString& message, int duration = 8000);
    
    // Generic method
    void ShowNotification(const wxString& title, const wxString& message, 
                         NotificationType type, int duration = 5000);
    
    // Management
    void SetParentWindow(wxWindow* parent);
    wxWindow* GetParentWindow() const { return m_parentWindow; }
    void ClearAll();
    void RemoveNotification(NotificationToast* toast);
    
    // Configuration
    void SetMaxNotifications(int max) { m_maxNotifications = max; }
    void SetStackingOffset(int offset) { m_stackingOffset = offset; }
    void SetMargins(int horizontal, int vertical) { 
        m_marginHorizontal = horizontal; 
        m_marginVertical = vertical; 
    }
    
private:
    NotificationSystem() = default;
    ~NotificationSystem() = default;
    NotificationSystem(const NotificationSystem&) = delete;
    NotificationSystem& operator=(const NotificationSystem&) = delete;
    
    void RepositionNotifications();
    wxPoint CalculatePosition(int index) const;
    void EnforceMaxNotifications();
    
    wxWindow* m_parentWindow = nullptr;
    std::vector<std::unique_ptr<NotificationToast>> m_notifications;
    
    // Configuration
    int m_maxNotifications = 5;
    int m_stackingOffset = 10;
    int m_marginHorizontal = 20;
    int m_marginVertical = 70;
};

// Convenience macros for easy usage
#define NOTIFY_INFO(title, message) NotificationSystem::Instance().ShowInfo(title, message)
#define NOTIFY_SUCCESS(title, message) NotificationSystem::Instance().ShowSuccess(title, message)
#define NOTIFY_WARNING(title, message) NotificationSystem::Instance().ShowWarning(title, message)
#define NOTIFY_ERROR(title, message) NotificationSystem::Instance().ShowError(title, message)
