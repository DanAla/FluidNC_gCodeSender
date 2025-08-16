/**
 * gui/MainFrame_Layouts.cpp
 *
 * Layout management implementation for the MainFrame class.
 *
 * This file is part of the MainFrame refactoring to split its functionality
 * into smaller, more manageable parts.
 */

#include "MainFrame.h"
#include "core/SimpleLogger.h"
#include "core/StateManager.h"
#include "core/StringUtils.h"
#include "NotificationSystem.h"
#include "DROPanel.h"
#include "JogPanel.h"
#include "MachineManagerPanel.h"
#include "ConsolePanel.h"
#include "GCodeEditor.h"
#include "MacroPanel.h"
#include "SVGViewer.h"
#include "MachineVisualizationPanel.h"

#include <wx/wx.h> // For wxRect, wxGetDisplaySize etc.
#include <wx/artprov.h>


// AUI-based panel creation
void MainFrame::CreatePanels()
{
    // Clear any existing panels
    m_panels.clear();
    
    try {
        // Create panels as direct children of the main frame
        
        // G-Code Editor
        PanelInfo gcodeInfo;
        gcodeInfo.id = PANEL_GCODE_EDITOR;
        gcodeInfo.name = "gcode_editor";
        gcodeInfo.title = "G-code Editor";
        gcodeInfo.panel = new GCodeEditor(this);
        gcodeInfo.defaultVisible = false;  // No default visibility - state dependent
        gcodeInfo.defaultPosition = "";     // No default position - state dependent
        gcodeInfo.defaultSize = wxSize(600, 400);
        m_panels.push_back(gcodeInfo);
        
        // DRO Panel
        PanelInfo droInfo;
        droInfo.id = PANEL_DRO;
        droInfo.name = "dro";
        droInfo.title = "Digital Readout";
        droInfo.panel = new DROPanel(this, nullptr); // nullptr for ConnectionManager
        droInfo.defaultVisible = false;  // No default visibility - state dependent
        droInfo.defaultPosition = "";     // No default position - state dependent
        droInfo.defaultSize = wxSize(250, 200);
        m_panels.push_back(droInfo);
        
        // Jog Panel
        PanelInfo jogInfo;
        jogInfo.id = PANEL_JOG;
        jogInfo.name = "jog";
        jogInfo.title = "Jogging Controls";
        jogInfo.panel = new JogPanel(this, nullptr); // nullptr for ConnectionManager
        jogInfo.defaultVisible = false;  // No default visibility - state dependent
        jogInfo.defaultPosition = "";     // No default position - state dependent
        jogInfo.defaultSize = wxSize(250, 300);
        m_panels.push_back(jogInfo);
        
        // Console Panel
        PanelInfo consoleInfo;
        consoleInfo.id = PANEL_CONSOLE;
        consoleInfo.name = "console";
        consoleInfo.title = "Terminal Console";
        consoleInfo.panel = new ConsolePanel(this);
        consoleInfo.canClose = true;             // Allow closing
        consoleInfo.defaultVisible = false;      // No default visibility - state dependent
        consoleInfo.defaultPosition = "";        // No default position - state dependent
        consoleInfo.defaultSize = wxSize(800, 150);
        m_panels.push_back(consoleInfo);
        
        // Machine Manager Panel
        PanelInfo machineInfo;
        machineInfo.id = PANEL_MACHINE_MANAGER;
        machineInfo.name = "machine_manager";
        machineInfo.title = "Machine Manager";
        machineInfo.panel = new MachineManagerPanel(this);
        machineInfo.defaultVisible = false;  // No default visibility - state dependent
        machineInfo.defaultPosition = "";      // No default position - state dependent
        machineInfo.defaultSize = wxSize(300, 400);
        m_panels.push_back(machineInfo);
        
        // Macro Panel
        PanelInfo macroInfo;
        macroInfo.id = PANEL_MACRO;
        macroInfo.name = "macro";
        macroInfo.title = "Macro Panel";
        macroInfo.panel = new MacroPanel(this);
        macroInfo.defaultVisible = false;  // No default visibility - state dependent
        macroInfo.defaultPosition = "";      // No default position - state dependent
        macroInfo.defaultSize = wxSize(300, 200);
        m_panels.push_back(macroInfo);
        
        // SVG Viewer
        PanelInfo svgInfo;
        svgInfo.id = PANEL_SVG_VIEWER;
        svgInfo.name = "svg_viewer";
        svgInfo.title = "SVG Viewer";
        svgInfo.panel = new SVGViewer(this);
        svgInfo.defaultVisible = false;  // No default visibility - state dependent
        svgInfo.defaultPosition = "";      // No default position - state dependent
        svgInfo.defaultSize = wxSize(400, 400);
        m_panels.push_back(svgInfo);
        
        // Machine Visualization Panel
        PanelInfo machineVisInfo;
        machineVisInfo.id = PANEL_MACHINE_VISUALIZATION;
        machineVisInfo.name = "machine_visualization";
        machineVisInfo.title = "Machine Visualization";
        machineVisInfo.panel = new MachineVisualizationPanel(this);
        machineVisInfo.defaultVisible = false;  // No default visibility - state dependent
        machineVisInfo.defaultPosition = "";      // No default position - state dependent
        machineVisInfo.defaultSize = wxSize(500, 400);
        m_panels.push_back(machineVisInfo);
        
    } catch (const std::exception& e) {
        // If panel creation fails, create a single error panel
        wxPanel* errorPanel = new wxPanel(this, wxID_ANY);
        wxStaticText* errorText = new wxStaticText(errorPanel, wxID_ANY, 
            wxString::Format("Panel creation error: %s\n\nThe application will still run with limited functionality.", e.what()));
        wxBoxSizer* errorSizer = new wxBoxSizer(wxVERTICAL);
        errorSizer->Add(errorText, 1, wxALL | wxCENTER, 20);
        errorPanel->SetSizer(errorSizer);
        
        // Create minimal panel info for error panel
        PanelInfo errorInfo;
        errorInfo.id = PANEL_GCODE_EDITOR; // Use existing ID
        errorInfo.name = "error";
        errorInfo.title = "Error";
        errorInfo.panel = errorPanel;
        errorInfo.defaultVisible = true;
        errorInfo.defaultPosition = "center";
        m_panels.clear();
        m_panels.push_back(errorInfo);
    }
}

void MainFrame::ResetLayout() {
    LOG_INFO("ResetLayout: Resetting panel layout without destroying panels");
    
    // Hide all panels first
    for (auto& panelInfo : m_panels) {
        wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
        if (pane.IsOk()) {
            pane.Hide();
            LOG_INFO(wxString::Format("Hidden panel: %s", panelInfo.name).ToStdString());
        }
    }
    
    // Update AUI manager to apply the hide operations
    m_auiManager.Update();
    
    // Use connection-first layout as the default
    SetupConnectionFirstLayout();
    
    m_auiManager.Update();
    UpdateMenuItems();
    
    // Notify user that layout was reset
    NOTIFY_SUCCESS("Layout Reset", "All panels have been restored to their default positions.");
}

void MainFrame::SaveCurrentLayout() {
    // TODO: Save AUI perspective to config
    // wxString perspective = m_auiManager.SavePerspective();
    // StateManager::SavePerspective(perspective);
}

void MainFrame::LoadSavedLayout() {
    // TODO: Load AUI perspective from config
    // wxString perspective = StateManager::LoadPerspective();
    // if (!perspective.IsEmpty()) {
    //     m_auiManager.LoadPerspective(perspective, true);
    // }
}

void MainFrame::RestoreWindowGeometry()
{
    try {
        // Get the saved window layout from StateManager
        WindowLayout layout = StateManager::getInstance().getWindowLayout("MainFrame");
        
        // If we have a saved layout, restore it
        if (layout.windowId == "MainFrame" && layout.width > 0 && layout.height > 0) {
            // Validate the position is on screen
            wxRect displayRect = wxGetDisplaySize();
            
            // Ensure window fits on screen
            int x = std::max(0, std::min(layout.x, displayRect.GetWidth() - layout.width));
            int y = std::max(0, std::min(layout.y, displayRect.GetHeight() - layout.height));
            
            // Ensure minimum size
            int width = std::max(400, layout.width);
            int height = std::max(300, layout.height);
            
            // Set position and size
            SetPosition(wxPoint(x, y));
            SetSize(wxSize(width, height));
            
            // Handle maximized state
            if (layout.maximized) {
                Maximize(true);
            }
            
            LOG_INFO("Restored MainFrame geometry: " + std::to_string(x) + "," + std::to_string(y) + " " + std::to_string(width) + "x" + std::to_string(height));
            
            // If position was corrected, save the corrected values immediately
            if (x != layout.x || y != layout.y) {
                CallAfter([this]() {
                    SaveWindowGeometry(); // Save the corrected position
                });
            }
        } else {
            // No saved layout, use defaults
            SetSize(1200, 800);
            Center();
            LOG_INFO("Using default MainFrame geometry (no saved layout found)");
        }
    } catch (const std::exception& e) {
        // If restoration fails, fall back to defaults
        SetSize(1200, 800);
        Center();
        LOG_ERROR("Failed to restore MainFrame geometry: " + std::string(e.what()) + " - using defaults");
    }
}

void MainFrame::SaveWindowGeometry()
{
    try {
        WindowLayout layout;
        layout.windowId = "MainFrame";
        
        // Get current window state
        layout.maximized = IsMaximized();
        
        if (!layout.maximized) {
            // Only save position/size if not maximized
            wxPoint pos = GetPosition();
            wxSize size = GetSize();
            
            layout.x = pos.x;
            layout.y = pos.y;
            layout.width = size.GetWidth();
            layout.height = size.GetHeight();
        } else {
            // For maximized window, save the normal (restored) size
            wxRect normalRect = GetRect();
            layout.x = normalRect.x;
            layout.y = normalRect.y;
            layout.width = normalRect.width;
            layout.height = normalRect.height;
        }
        
        layout.visible = IsShown();
        layout.docked = false; // MainFrame is never docked
        layout.dockingSide = "center";
        
        // Save to StateManager
        StateManager::getInstance().saveWindowLayout(layout);
        
        LOG_INFO("Saved MainFrame geometry: " + std::to_string(layout.x) + "," + std::to_string(layout.y) + 
                " " + std::to_string(layout.width) + "x" + std::to_string(layout.height) + 
                (layout.maximized ? " (maximized)" : ""));
                
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save MainFrame geometry: " + std::string(e.what()));
    }
}

// Setup AUI manager and add panels
void MainFrame::SetupAuiManager()
{
    // Add ALL panels to AUI manager, even if they start hidden
    // This ensures they can be shown later by layout restoration
    for (auto& panelInfo : m_panels) {
        AddPanelToAui(panelInfo);
        
        // Hide panels that shouldn't be visible by default
        if (!panelInfo.defaultVisible) {
            wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
            if (pane.IsOk()) {
                pane.Show(false);
            }
        }
    }
}

// Create default AUI layout
void MainFrame::CreateDefaultLayout()
{
    // This will be called after all panels are added to AUI
    // The AUI manager will automatically arrange them based on
    // the positions specified in AddPanelToAui
}

// Setup Connection-First layout - only show essential panels for connection
void MainFrame::SetupConnectionFirstLayout()
{
    LOG_INFO("Setting up Connection-First layout...");
    
    // First hide all panels
    for (auto& panelInfo : m_panels) {
        wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
        if (pane.IsOk()) {
            pane.Hide();
            LOG_INFO(wxString::Format("Hidden panel: %s", panelInfo.name).ToStdString());
        }
    }
    
    // Then show and position only the panels we want
    for (auto& panelInfo : m_panels) {
        wxAuiPaneInfo* pane = &m_auiManager.GetPane(panelInfo.name);
        if (!pane->IsOk()) {
            // Panel not in AUI manager yet, add it
            auto newPane = AddPanelToAui(panelInfo);
            pane = &m_auiManager.GetPane(panelInfo.name);
        }
        
        if (panelInfo.id == PANEL_MACHINE_MANAGER) {
            LOG_INFO("Configuring Machine Manager panel");
            pane->Caption(panelInfo.title)
                .BestSize(480, 600)
                .MinSize(300, 400)
                .Left()        // Dock to left side
                .Layer(0)      // Base layer
                .Row(0)
                .Show();       // Make visible
        } else if (panelInfo.id == PANEL_CONSOLE) {
            LOG_INFO("Configuring Console panel");
            pane->Caption(panelInfo.title)
                .BestSize(720, 600)
                .MinSize(400, 150)
                .Center()      // Fill center area
                .Layer(0)      // Same layer
                .Row(0)
                .Show();       // Make visible
        }
        // All other panels remain hidden
    }
    
    // Update the AUI manager to apply the layout
    m_auiManager.Update();
    
    // DO NOT save immediately - let user adjust first
    // SaveConnectionFirstLayout(); // Removed - only save after user adjusts
}

// Add a panel to the AUI manager
wxAuiPaneInfo MainFrame::AddPanelToAui(PanelInfo& panelInfo)
{
    // Safety check: Don't add if pane already exists
wxAuiPaneInfo existingPane = m_auiManager.GetPane(panelInfo.name);
    if (existingPane.IsOk()) {
        // Pane already exists, just return it
            LOG_INFO(wxString::Format("Panel '%s' already in AUI manager", panelInfo.name).ToStdString());
        return existingPane;
    }
    
    LOG_INFO(wxString::Format("Adding panel '%s' to AUI manager", panelInfo.name).ToStdString());
    
    // Create new pane info with default settings
    wxAuiPaneInfo paneInfo;
    paneInfo.Name(panelInfo.name)
            .Caption(panelInfo.title)
            .BestSize(panelInfo.defaultSize)
            .MinSize(wxSize(200, 150))
            .CloseButton(panelInfo.canClose)
            .MaximizeButton(true)
            .MinimizeButton(true)
            .PinButton(true)
            .Dock()
            .Resizable(true)
            .Floatable(true)
            .Movable(true)
            .Center()  // Default to center, position will be adjusted later
            .Layer(0)
            .Hide();   // Start hidden, will be shown if needed
    
    // Perform the actual add operation
    m_auiManager.AddPane(panelInfo.panel, paneInfo);
    m_auiManager.Update();
    
    LOG_INFO(wxString::Format("Added panel '%s' to AUI manager", panelInfo.name).ToStdString());
    
    // Get and return the actual pane info that was added
    return m_auiManager.GetPane(panelInfo.name);
}

// Save the Connection-First layout state
void MainFrame::SaveConnectionFirstLayout()
{
    try {
        // Save the AUI perspective string for the connection-first layout
        wxString perspective = m_auiManager.SavePerspective();
        
        // Save to StateManager with a special key for connection-first layout
        StateManager::getInstance().setValue("ConnectionFirstLayout", perspective.ToStdString());
        
        LOG_INFO("Saved Connection-First layout perspective");
        
    } catch (const std::exception& e) {
LOG_ERROR("Failed to save Connection-First layout: " + StringUtils::enforceASCII(e.what()));
    }
}

// Restore the Connection-First layout state
void MainFrame::RestoreConnectionFirstLayout()
{
    LOG_INFO("RestoreConnectionFirstLayout: Starting layout restoration");
    
    try {
        // Try to load saved ConnectionFirstLayout perspective first
        std::string savedPerspective = StateManager::getInstance().getValue<std::string>("ConnectionFirstLayout", "");
        
        LOG_INFO("RestoreConnectionFirstLayout: Saved perspective length: " + std::to_string(savedPerspective.length()));
        
        if (!savedPerspective.empty()) {
            // Load the saved perspective (includes splitter positions)
wxString perspective = TO_WX(savedPerspective);
            LOG_INFO("RestoreConnectionFirstLayout: Attempting to load saved perspective");
            
            if (m_auiManager.LoadPerspective(perspective, true)) {
                LOG_INFO("RestoreConnectionFirstLayout: Successfully loaded saved perspective - PRESERVING splitter positions");
                m_auiManager.Update();
                UpdateMenuItems();
                
                // Show notification about layout restoration
                NOTIFY_SUCCESS("Connection Layout Restored", "Saved layout with preserved splitter positions restored.");
                LOG_INFO("RestoreConnectionFirstLayout: Layout restoration completed successfully");
                return; // CRITICAL: Exit here to preserve the loaded layout
            } else {
                LOG_INFO("RestoreConnectionFirstLayout: LoadPerspective failed, falling back to manual setup");
            }
        } else {
            LOG_INFO("RestoreConnectionFirstLayout: No saved perspective found, using manual setup");
        }
        
        // Manual fallback setup - ensure required panels exist in AUI manager
        LOG_INFO("RestoreConnectionFirstLayout: Setting up manual fallback layout");
        
        // Only add panels if they don't exist (first-time setup)
        PanelInfo* machineManagerInfo = FindPanelInfo(PANEL_MACHINE_MANAGER);
        if (machineManagerInfo) {
            wxAuiPaneInfo& pane = m_auiManager.GetPane(machineManagerInfo->name);
            if (!pane.IsOk()) {
                LOG_INFO("RestoreConnectionFirstLayout: Adding Machine Manager panel to AUI manager");
                // Panel not in AUI manager, add it with default settings (first-time setup only)
                AddPanelToAui(*machineManagerInfo);
            } else {
                LOG_INFO("RestoreConnectionFirstLayout: Machine Manager panel already in AUI manager");
            }
        } else {
            LOG_ERROR("RestoreConnectionFirstLayout: Machine Manager panel not found!");
        }
        
        PanelInfo* consoleInfo = FindPanelInfo(PANEL_CONSOLE);
        if (consoleInfo) {
            wxAuiPaneInfo& pane = m_auiManager.GetPane(consoleInfo->name);
            if (!pane.IsOk()) {
                LOG_INFO("RestoreConnectionFirstLayout: Adding Console panel to AUI manager");
                // Panel not in AUI manager, add it with default settings (first-time setup only)
                AddPanelToAui(*consoleInfo);
            } else {
                LOG_INFO("RestoreConnectionFirstLayout: Console panel already in AUI manager");
            }
        } else {
            LOG_ERROR("RestoreConnectionFirstLayout: Console panel not found!");
        }
        
        // If no saved perspective or loading failed, set up the layout manually
        // First minimize non-essential panels instead of hiding them
        MinimizeNonEssentialPanels();
        
        // Ensure Machine Manager and Console are visible, but preserve user adjustments
        for (auto& panelInfo : m_panels) {
            if (panelInfo.id == PANEL_MACHINE_MANAGER || panelInfo.id == PANEL_CONSOLE) {
                wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
                if (pane.IsOk()) {
                    // Only show the pane - DO NOT reset size/position (preserves user splitter adjustments)
                    pane.Show(true);
                } else {
                    // Panel not in AUI manager, add it with default settings (first-time setup only)
                    wxAuiPaneInfo paneInfo;
                    if (panelInfo.id == PANEL_MACHINE_MANAGER) {
                        paneInfo.Name(panelInfo.name)
                               .Caption(panelInfo.title)
                               .BestSize(480, 600)
                               .MinSize(300, 400)
                               .Left()
                               .Layer(0)
                               .Row(0);
                    } else {
                        paneInfo.Name(panelInfo.name)
                               .Caption(panelInfo.title)
                               .BestSize(720, 600)
                               .MinSize(400, 150)
                               .Center()
                               .Layer(0)
                               .Row(0);
                    }
                    paneInfo.CloseButton(panelInfo.canClose)
                           .MaximizeButton(true)
                           .MinimizeButton(true)
                           .PinButton(true)
                           .Dock()
                           .Resizable(true)
                           .Movable(true)
                           .Floatable(true);
                    m_auiManager.AddPane(panelInfo.panel, paneInfo);
                }
            }
        }
        
        m_auiManager.Update();
        UpdateMenuItems();
        
        // Save this layout for future use
        SaveConnectionFirstLayout();
        
        // Show notification about layout restoration
        NOTIFY_SUCCESS("Connection Layout Restored", "Essential panels (Machine Manager + Console) are now active. Other panels are minimized.");
        
    } catch (const std::exception& e) {
LOG_ERROR("Failed to restore Connection-First layout: " + StringUtils::enforceASCII(e.what()));
        
        // Fallback to creating a new connection-first layout
        SetupConnectionFirstLayout();
        m_auiManager.Update();
        UpdateMenuItems();
    }
}

// Setup G-Code layout - G-code Editor on left, Machine Visualization on right
void MainFrame::SetupGCodeLayout()
{
    LOG_INFO("Setting up G-Code layout...");
    
    // First hide all panels
    for (auto& panelInfo : m_panels) {
        wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
        if (pane.IsOk()) {
            pane.Hide();
            LOG_INFO(wxString::Format("Hidden panel: %s", panelInfo.name).ToStdString());
        }
    }
    
    // Then show and position only the panels we want
    for (auto& panelInfo : m_panels) {
        wxAuiPaneInfo* pane = &m_auiManager.GetPane(panelInfo.name);
        if (!pane->IsOk()) {
            // Panel not in AUI manager yet, add it
            auto newPane = AddPanelToAui(panelInfo);
            pane = &m_auiManager.GetPane(panelInfo.name);
        }
        
        if (panelInfo.id == PANEL_GCODE_EDITOR) {
            LOG_INFO("Configuring G-Code Editor panel");
            pane->Caption(panelInfo.title)
                .BestSize(720, 600)
                .MinSize(400, 300)
                .Left()        // Dock to left side
                .Layer(0)      // Base layer
                .Row(0)
                .Show();       // Make visible
        } else if (panelInfo.id == PANEL_MACHINE_VISUALIZATION) {
            LOG_INFO("Configuring Machine Visualization panel");
            pane->Caption(panelInfo.title)
                .BestSize(480, 600)
                .MinSize(300, 200)
                .Center()      // Fill center area
                .Layer(0)      // Same layer
                .Row(0)
                .Show();       // Make visible
        }
        // All other panels remain hidden
    }
    
    // Update the AUI manager to apply the layout
    m_auiManager.Update();
    
    // DO NOT save immediately - let user adjust first
    // SaveGCodeLayout(); // Removed - only save after user adjusts
}

// Save the G-Code layout state
void MainFrame::SaveGCodeLayout()
{
    try {
        // Save the AUI perspective string for the G-code layout
        wxString perspective = m_auiManager.SavePerspective();
        
        // Save to StateManager with a special key for G-code layout
        StateManager::getInstance().setValue("GCodeLayout", perspective.ToStdString());
        
        LOG_INFO("Saved G-Code layout perspective");
        
    } catch (const std::exception& e) {
LOG_ERROR("Failed to save G-Code layout: " + StringUtils::enforceASCII(e.what()));
    }
}

// Restore the G-Code layout state
void MainFrame::RestoreGCodeLayout()
{
    LOG_INFO("RestoreGCodeLayout: Starting layout restoration");
    
    try {
        // Try to load saved GCodeLayout perspective first
        std::string savedPerspective = StateManager::getInstance().getValue<std::string>("GCodeLayout", "");
        
        LOG_INFO("RestoreGCodeLayout: Saved perspective length: " + std::to_string(savedPerspective.length()));
        
        if (!savedPerspective.empty()) {
            // Load the saved perspective (includes splitter positions)
            wxString perspective = wxString::FromUTF8(savedPerspective.c_str());
            LOG_INFO("RestoreGCodeLayout: Attempting to load saved perspective");
            
            if (m_auiManager.LoadPerspective(perspective, true)) {
                LOG_INFO("RestoreGCodeLayout: Successfully loaded saved perspective - PRESERVING splitter positions");
                m_auiManager.Update();
                UpdateMenuItems();
                
                // CRITICAL: Connect G-code Editor and Machine Visualization panels even when restoring saved layout
                ConnectGCodePanels();
                
                // Show notification about layout restoration
                NOTIFY_SUCCESS("G-Code Layout Restored", "Saved layout with preserved splitter positions restored.");
                LOG_INFO("RestoreGCodeLayout: Layout restoration completed successfully");
                return; // CRITICAL: Exit here to preserve the loaded layout
            } else {
                LOG_INFO("RestoreGCodeLayout: LoadPerspective failed, falling back to manual setup");
            }
        } else {
            LOG_INFO("RestoreGCodeLayout: No saved perspective found, using manual setup");
        }
        
        // If no saved perspective or loading failed, set up the G-Code layout from scratch
        // This ensures proper G-Code layout positioning instead of generic AddPanelToAui
        
        // First hide all other panels for focused G-code work
        for (auto& panelInfo : m_panels) {
            if (panelInfo.id != PANEL_GCODE_EDITOR && panelInfo.id != PANEL_MACHINE_VISUALIZATION) {
                wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
                if (pane.IsOk() && pane.IsShown()) {
                    pane.Show(false);
                    LOG_INFO("Hid non-G-code panel: " + panelInfo.name.ToStdString());
                }
            }
        }
        
        // Ensure G-code Editor and Machine Visualization are visible, but preserve user adjustments
        for (auto& panelInfo : m_panels) {
            if (panelInfo.id == PANEL_GCODE_EDITOR || panelInfo.id == PANEL_MACHINE_VISUALIZATION) {
                wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
                if (pane.IsOk()) {
                    // Only show the pane - DO NOT reset size/position (preserves user splitter adjustments)
                    pane.Show(true);
                } else {
                    // Panel not in AUI manager, add it with G-Code layout specific settings (first-time setup only)
                    wxAuiPaneInfo paneInfo;
                    if (panelInfo.id == PANEL_GCODE_EDITOR) {
                        paneInfo.Name(panelInfo.name)
                               .Caption(panelInfo.title)
                               .BestSize(720, 600)
                               .MinSize(400, 300)
                               .Left()      // G-Code layout: Editor on left
                               .Layer(0)
                               .Row(0);
                    } else {
                        paneInfo.Name(panelInfo.name)
                               .Caption(panelInfo.title)
                               .BestSize(480, 600)
                               .MinSize(300, 200)
                               .Center()    // G-Code layout: Visualization in center
                               .Layer(0)
                               .Row(0);
                    }
                    paneInfo.CloseButton(panelInfo.canClose)
                           .MaximizeButton(true)
                           .MinimizeButton(true)
                           .PinButton(true)
                           .Dock()
                           .Resizable(true)
                           .Movable(true)
                           .Floatable(true);
                    m_auiManager.AddPane(panelInfo.panel, paneInfo);
                }
            }
        }
        
        
        m_auiManager.Update();
        UpdateMenuItems();
        
        // Connect G-code Editor and Machine Visualization panels
        ConnectGCodePanels();
        
        // Save this layout for future use
        SaveGCodeLayout();
        
        // Show notification about layout restoration
        NOTIFY_SUCCESS("G-Code Layout Restored", "G-code editing panels (Editor + Machine Visualization) are now active. Other panels are minimized.");
        
    } catch (const std::exception& e) {
LOG_ERROR("Failed to restore G-Code layout: " + StringUtils::enforceASCII(e.what()));
        
        // Fallback to creating a new G-code layout
        SetupGCodeLayout();
        m_auiManager.Update();
        UpdateMenuItems();
    }
}

// Connect G-Code Editor and Machine Visualization panels for real-time updates
void MainFrame::ConnectGCodePanels() {
    try {
        // Find the G-Code Editor panel
        PanelInfo* gcodeEditorInfo = FindPanelInfo(PANEL_GCODE_EDITOR);
        if (!gcodeEditorInfo) {
            LOG_WARNING("ConnectGCodePanels: G-Code Editor panel not found");
            return;
        }
        
        GCodeEditor* gcodeEditor = dynamic_cast<GCodeEditor*>(gcodeEditorInfo->panel);
        if (!gcodeEditor) {
            LOG_ERROR("ConnectGCodePanels: G-Code Editor panel cast failed");
            return;
        }
        
        // Find the Machine Visualization panel
        PanelInfo* machineVisInfo = FindPanelInfo(PANEL_MACHINE_VISUALIZATION);
        if (!machineVisInfo) {
            LOG_WARNING("ConnectGCodePanels: Machine Visualization panel not found");
            return;
        }
        
        MachineVisualizationPanel* machineVis = dynamic_cast<MachineVisualizationPanel*>(machineVisInfo->panel);
        if (!machineVis) {
            LOG_ERROR("ConnectGCodePanels: Machine Visualization panel cast failed");
            return;
        }
        
        // Set up the callback from G-Code Editor to Machine Visualization
        gcodeEditor->SetTextChangeCallback([machineVis](const std::string& gcodeText) {
            try {
                LOG_INFO("MainFrame callback: Received G-code text of length: " + std::to_string(gcodeText.length()));
        // Update visualization with the new G-code content (ensuring ASCII-only)
                machineVis->SetGCodeContent(TO_WX(gcodeText));
                
                LOG_INFO("G-Code visualization updated from editor change");
                
            } catch (const std::exception& e) {
LOG_ERROR("Failed to update G-Code visualization: " + StringUtils::enforceASCII(e.what()));
            }
        });
        
        // Also update visualization with current G-code content immediately
        std::string currentGCode_std = gcodeEditor->GetText();
        if (!currentGCode_std.empty()) {
            machineVis->SetGCodeContent(TO_WX(currentGCode_std));
        }
        
        LOG_INFO("Successfully connected G-Code Editor and Machine Visualization panels");
        
        // Show notification about the connection
        NOTIFY_SUCCESS("G-Code Panels Connected", 
            "G-Code Editor is now linked to Machine Visualization. Changes will update in real-time.");
        
    } catch (const std::exception& e) {
        LOG_ERROR("ConnectGCodePanels failed: " + std::string(e.what()));
        
        // Show error notification to user
        NOTIFY_ERROR("G-Code Panel Connection Failed", 
            "Could not connect G-Code Editor to Machine Visualization. Real-time updates may not work.");
    }
}

// Smart layout saving based on current context
void MainFrame::SaveCurrentLayoutBasedOnContext() {
    try {
        // Determine which layout we're in by checking visible panels
        bool hasGCodeEditor = IsPanelVisible(PANEL_GCODE_EDITOR);
        bool hasMachineVis = IsPanelVisible(PANEL_MACHINE_VISUALIZATION);
        bool hasMachineManager = IsPanelVisible(PANEL_MACHINE_MANAGER);
        bool hasConsole = IsPanelVisible(PANEL_CONSOLE);
        
        // Save appropriate layout based on current configuration
        if (hasGCodeEditor && hasMachineVis) {
            // Likely in G-Code layout - save it
            SaveGCodeLayout();
        } else if (hasMachineManager && hasConsole) {
            // Likely in Connection layout - save it
            SaveConnectionFirstLayout();
        }
        // If we can't determine context, don't save to avoid corrupting layouts
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save current layout based on context: " + std::string(e.what()));
    }
}


void MainFrame::MinimizeNonEssentialPanels() {
    // Hide all non-essential panels (they can be restored later)
    for (auto& panelInfo : m_panels) {
        if (panelInfo.id != PANEL_MACHINE_MANAGER && panelInfo.id != PANEL_CONSOLE) {
            wxAuiPaneInfo& pane = m_auiManager.GetPane(panelInfo.name);
            if (pane.IsOk() && pane.IsShown()) {
                // Hide the panel (user can restore via menu or when machine connects)
                pane.Show(false);
LOG_INFO("Hid non-essential panel: " + TO_ASCII(panelInfo.name));
            }
        }
    }
}


