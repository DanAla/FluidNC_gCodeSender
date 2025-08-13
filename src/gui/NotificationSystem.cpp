/**
 * gui/NotificationSystem.cpp
 * Implementation of modern notification system with floating toast notifications
 */

#include "NotificationSystem.h"
#include "core/SimpleLogger.h"
#include <wx/artprov.h>
#include <wx/dcbuffer.h>
#include <wx/app.h>
#include <algorithm>

// Toast window events
wxBEGIN_EVENT_TABLE(NotificationToast, wxFrame)
    EVT_TIMER(wxID_ANY, NotificationToast::OnTimer)
    EVT_ENTER_WINDOW(NotificationToast::OnMouseEnter)
    EVT_LEAVE_WINDOW(NotificationToast::OnMouseLeave)
    EVT_BUTTON(wxID_CLOSE, NotificationToast::OnCloseButton)
    EVT_PAINT(NotificationToast::OnPaint)
    EVT_SIZE(NotificationToast::OnSize)
wxEND_EVENT_TABLE()

NotificationToast::NotificationToast(wxWindow* parent, const wxString& title, 
                                     const wxString& message, NotificationType type, int duration)
    : wxFrame(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(320, 80), 
              wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxBORDER_NONE)
    , m_title(title)
    , m_message(message) 
    , m_type(type)
    , m_duration(duration)
    , m_autoHideTimer(this)
    , m_animationTimer(this)
    , m_panel(nullptr)
    , m_iconBitmap(nullptr)
    , m_titleText(nullptr)
    , m_messageText(nullptr)
    , m_closeButton(nullptr)
    , m_isAnimating(false)
    , m_isPaused(false)
    , m_animationStep(0)
    , m_targetAlpha(255)
{
    // Set background style required for wxAutoBufferedPaintDC
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(GetBackgroundColor());
    CreateControls();
    
    // Start with transparent and animate in
    SetTransparent(0);
    AnimateIn();
}

NotificationToast::~NotificationToast()
{
    m_autoHideTimer.Stop();
    m_animationTimer.Stop();
}

void NotificationToast::CreateControls()
{
    // Main panel with rounded corners effect
    m_panel = new wxPanel(this, wxID_ANY);
    m_panel->SetBackgroundColour(GetBackgroundColor());
    
    // Create icon
    wxBitmap iconBmp = GetIcon();
    m_iconBitmap = new wxStaticBitmap(m_panel, wxID_ANY, iconBmp);
    
    // Create title text
    m_titleText = new wxStaticText(m_panel, wxID_ANY, m_title);
    wxFont titleFont = m_titleText->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleFont.SetPointSize(titleFont.GetPointSize() + 1);
    m_titleText->SetFont(titleFont);
    m_titleText->SetForegroundColour(GetTextColor());
    
    // Create message text  
    m_messageText = new wxStaticText(m_panel, wxID_ANY, m_message);
    m_messageText->SetForegroundColour(GetTextColor());
    m_messageText->Wrap(250); // Wrap long messages
    
    // Create close button
    m_closeButton = new wxButton(m_panel, wxID_CLOSE, "X", wxDefaultPosition, wxSize(20, 20));
    m_closeButton->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    
    // Layout
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Icon
    mainSizer->Add(m_iconBitmap, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);
    
    // Text area
    wxBoxSizer* textSizer = new wxBoxSizer(wxVERTICAL);
    textSizer->Add(m_titleText, 0, wxEXPAND | wxTOP, 4);
    textSizer->Add(m_messageText, 1, wxEXPAND | wxTOP, 2);
    
    mainSizer->Add(textSizer, 1, wxEXPAND | wxTOP | wxBOTTOM, 8);
    
    // Close button
    mainSizer->Add(m_closeButton, 0, wxALL | wxALIGN_TOP, 4);
    
    m_panel->SetSizer(mainSizer);
    
    // Frame sizer
    wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(m_panel, 1, wxEXPAND | wxALL, 2); // Border for rounded effect
    SetSizer(frameSizer);
    
    // Calculate proper size based on content
    wxSize textSize = m_messageText->GetBestSize();
    wxSize minSize(320, std::max(80, textSize.GetHeight() + 50));
    SetMinSize(minSize);
    SetSize(minSize);
    
    // Bind mouse events for hover detection
    Bind(wxEVT_ENTER_WINDOW, &NotificationToast::OnMouseEnter, this);
    Bind(wxEVT_LEAVE_WINDOW, &NotificationToast::OnMouseLeave, this);
    m_panel->Bind(wxEVT_ENTER_WINDOW, &NotificationToast::OnMouseEnter, this);
    m_panel->Bind(wxEVT_LEAVE_WINDOW, &NotificationToast::OnMouseLeave, this);
}

wxColour NotificationToast::GetBackgroundColor() const
{
    switch (m_type) {
        case NotificationType::INFO:
            return wxColour(240, 248, 255); // Light blue
        case NotificationType::SUCCESS:
            return wxColour(240, 255, 240); // Light green
        case NotificationType::WARNING:
            return wxColour(255, 248, 220); // Light yellow
        case NotificationType::ERROR_TYPE:
            return wxColour(255, 240, 240); // Light red
        default:
            return wxColour(248, 248, 248); // Light gray
    }
}

wxColour NotificationToast::GetBorderColor() const
{
    switch (m_type) {
        case NotificationType::INFO:
            return wxColour(70, 130, 180); // Steel blue
        case NotificationType::SUCCESS:
            return wxColour(34, 139, 34); // Forest green
        case NotificationType::WARNING:
            return wxColour(255, 165, 0); // Orange
        case NotificationType::ERROR_TYPE:
            return wxColour(220, 20, 60); // Crimson
        default:
            return wxColour(128, 128, 128); // Gray
    }
}

wxColour NotificationToast::GetTextColor() const
{
    switch (m_type) {
        case NotificationType::INFO:
            return wxColour(25, 25, 112); // Midnight blue
        case NotificationType::SUCCESS:
            return wxColour(0, 100, 0); // Dark green
        case NotificationType::WARNING:
            return wxColour(184, 134, 11); // Dark orange
        case NotificationType::ERROR_TYPE:
            return wxColour(139, 0, 0); // Dark red
        default:
            return wxColour(64, 64, 64); // Dark gray
    }
}

wxBitmap NotificationToast::GetIcon() const
{
    wxString artId;
    switch (m_type) {
        case NotificationType::INFO:
            artId = wxART_INFORMATION;
            break;
        case NotificationType::SUCCESS:
            artId = wxART_TICK_MARK;
            break;
        case NotificationType::WARNING:
            artId = wxART_WARNING;
            break;
        case NotificationType::ERROR_TYPE:
            artId = wxART_ERROR;
            break;
        default:
            artId = wxART_INFORMATION;
            break;
    }
    
    return wxArtProvider::GetBitmap(artId, wxART_MESSAGE_BOX, wxSize(24, 24));
}

void NotificationToast::StartAutoHide()
{
    if (m_duration > 0) {
        m_autoHideTimer.StartOnce(m_duration);
    }
}

void NotificationToast::PauseAutoHide()
{
    if (m_autoHideTimer.IsRunning()) {
        m_isPaused = true;
        m_autoHideTimer.Stop();
    }
}

void NotificationToast::ResumeAutoHide()
{
    if (m_isPaused) {
        m_isPaused = false;
        // Resume with remaining time (simplified - just restart)
        StartAutoHide();
    }
}

void NotificationToast::ForceClose()
{
    AnimateOut();
}

void NotificationToast::AnimateIn()
{
    m_isAnimating = true;
    m_animationStep = 0;
    m_targetAlpha = 255;
    m_animationTimer.Start(50); // 50ms intervals for smooth animation
}

void NotificationToast::AnimateOut()
{
    m_isAnimating = true;
    m_animationStep = 0;
    m_targetAlpha = 0;
    m_animationTimer.Start(30); // Faster fade out
}

void NotificationToast::OnTimer(wxTimerEvent& event)
{
    if (event.GetTimer().GetId() == m_autoHideTimer.GetId()) {
        // Auto-hide timer expired
        AnimateOut();
    } else if (event.GetTimer().GetId() == m_animationTimer.GetId()) {
        // Animation timer
        const int steps = 10;
        m_animationStep++;
        
        int alpha;
        if (m_targetAlpha == 255) {
            // Fade in
            alpha = (255 * m_animationStep) / steps;
        } else {
            // Fade out
            alpha = 255 - (255 * m_animationStep) / steps;
        }
        
        SetTransparent(std::max(0, std::min(255, alpha)));
        
        if (m_animationStep >= steps) {
            m_animationTimer.Stop();
            m_isAnimating = false;
            
            if (m_targetAlpha == 255) {
                // Animation in complete, start auto-hide timer
                StartAutoHide();
            } else {
                // Animation out complete, close window
                NotificationSystem::Instance().RemoveNotification(this);
            }
        }
    }
}

void NotificationToast::OnMouseEnter(wxMouseEvent& event)
{
    if (!m_isAnimating) {
        PauseAutoHide();
        SetTransparent(255); // Ensure full opacity on hover
    }
    event.Skip();
}

void NotificationToast::OnMouseLeave(wxMouseEvent& event)
{
    if (!m_isAnimating) {
        ResumeAutoHide();
    }
    event.Skip();
}

void NotificationToast::OnCloseButton(wxCommandEvent& event)
{
    ForceClose();
}

void NotificationToast::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);
    
    // Clear the entire background first (required for wxBG_STYLE_PAINT)
    wxSize size = GetSize();
    dc.SetBrush(wxBrush(GetBackgroundColor()));
    dc.SetPen(wxPen(GetBackgroundColor()));
    dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
    
    // Draw rounded border
    dc.SetPen(wxPen(GetBorderColor(), 2));
    dc.SetBrush(wxBrush(GetBackgroundColor()));
    dc.DrawRoundedRectangle(0, 0, size.GetWidth(), size.GetHeight(), 8);
}

void NotificationToast::OnSize(wxSizeEvent& event)
{
    Refresh();
    event.Skip();
}

// NotificationSystem implementation
NotificationSystem& NotificationSystem::Instance()
{
    static NotificationSystem instance;
    return instance;
}

void NotificationSystem::ShowInfo(const wxString& title, const wxString& message, int duration)
{
    ShowNotification(title, message, NotificationType::INFO, duration);
}

void NotificationSystem::ShowSuccess(const wxString& title, const wxString& message, int duration)
{
    ShowNotification(title, message, NotificationType::SUCCESS, duration);
}

void NotificationSystem::ShowWarning(const wxString& title, const wxString& message, int duration)
{
    ShowNotification(title, message, NotificationType::WARNING, duration);
}

void NotificationSystem::ShowError(const wxString& title, const wxString& message, int duration)
{
    ShowNotification(title, message, NotificationType::ERROR_TYPE, duration);
}

void NotificationSystem::ShowNotification(const wxString& title, const wxString& message, 
                                         NotificationType type, int duration)
{
    if (!m_parentWindow) {
        LOG_INFO("NotificationSystem: No parent window set - cannot show notification");
        return; // No parent window set
    }
    
    std::string logMsg = "NotificationSystem: Creating notification: " + title.ToStdString() + " - " + message.ToStdString();
    LOG_INFO(logMsg);
    
    // Create new notification
    std::string parentMsg = "Creating NotificationToast with parent " + std::to_string((uintptr_t)m_parentWindow);
    LOG_INFO(parentMsg);
    auto toast = std::make_unique<NotificationToast>(m_parentWindow, title, message, type, duration);
    LOG_INFO("NotificationToast created successfully");
    
    // CRITICAL: Show the toast window!
    toast->Show(true);
    LOG_INFO("NotificationToast Show() called - should be visible now");
    
    // Add to collection
    m_notifications.push_back(std::move(toast));
    std::string countMsg = "Toast added to collection. Total notifications: " + std::to_string(m_notifications.size());
    LOG_INFO(countMsg);
    
    // Enforce maximum notifications
    EnforceMaxNotifications();
    
    // Position all notifications
    RepositionNotifications();
    LOG_INFO("Notifications repositioned");
}

void NotificationSystem::SetParentWindow(wxWindow* parent)
{
    m_parentWindow = parent;
    LOG_INFO("NotificationSystem: Parent window set to " + std::to_string((uintptr_t)parent));
}

void NotificationSystem::ClearAll()
{
    for (auto& toast : m_notifications) {
        toast->ForceClose();
    }
    // The toasts will remove themselves via RemoveNotification
}

void NotificationSystem::RemoveNotification(NotificationToast* toast)
{
    auto it = std::find_if(m_notifications.begin(), m_notifications.end(),
        [toast](const std::unique_ptr<NotificationToast>& ptr) {
            return ptr.get() == toast;
        });
    
    if (it != m_notifications.end()) {
        LOG_INFO("Removing notification from collection");
        
        // Hide the window immediately
        (*it)->Hide();
        
        // Destroy the window immediately (should be safe since we're not in its event handler)
        (*it)->Destroy();
        
        // Remove from collection
        m_notifications.erase(it);
        LOG_INFO(std::string("Notifications remaining: ") + std::to_string(m_notifications.size()));
        
        // Reposition remaining notifications
        RepositionNotifications();
    }
}

void NotificationSystem::RepositionNotifications()
{
    for (size_t i = 0; i < m_notifications.size(); ++i) {
        wxPoint pos = CalculatePosition(static_cast<int>(i));
        m_notifications[i]->Move(pos);
    }
}

wxPoint NotificationSystem::CalculatePosition(int index) const
{
    if (!m_parentWindow) {
        return wxPoint(100, 100);
    }
    
    wxSize parentSize = m_parentWindow->GetSize();
    wxPoint parentPos = m_parentWindow->GetPosition();
    
    // Position in upper-right corner
    int x = parentPos.x + parentSize.GetWidth() - 320 - m_marginHorizontal;
    int y = parentPos.y + m_marginVertical;
    
    // Stack downwards
    y += index * (80 + m_stackingOffset);
    
    wxPoint calculatedPos(x, y);
    std::string posMsg = "Calculated position for notification " + std::to_string(index) + ": (" + 
                        std::to_string(calculatedPos.x) + ", " + std::to_string(calculatedPos.y) + 
                        "). Parent size: " + std::to_string(parentSize.GetWidth()) + "x" + std::to_string(parentSize.GetHeight()) +
                        ", Parent pos: (" + std::to_string(parentPos.x) + ", " + std::to_string(parentPos.y) + ")";
    LOG_INFO(posMsg);
    
    return calculatedPos;
}

void NotificationSystem::EnforceMaxNotifications()
{
    while (static_cast<int>(m_notifications.size()) > m_maxNotifications) {
        // Remove oldest notification (first in vector)
        if (!m_notifications.empty()) {
            m_notifications[0]->ForceClose();
            // It will remove itself via RemoveNotification
        }
    }
}
