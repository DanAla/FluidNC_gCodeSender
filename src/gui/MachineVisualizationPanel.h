/**
 * gui/MachineVisualizationPanel.h
 * Machine visualization panel for displaying G-code tool paths, machine position, and graphics
 */

#pragma once

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/graphics.h>
#include <vector>
#include <string>

struct GCodeLine {
    float startX, startY, startZ;
    float endX, endY, endZ;
    bool isRapid; // G0 rapid move vs G1 feed move
    wxColour color;
};

struct ToolPosition {
    float x, y, z;
    bool isValid;
    wxDateTime lastUpdate;
};

class MachineVisualizationPanel : public wxPanel
{
public:
    MachineVisualizationPanel(wxWindow* parent);
    ~MachineVisualizationPanel();

    // G-code visualization
    void LoadGCodeFile(const wxString& filename);
    void SetGCodeContent(const wxString& gcode);
    void ClearGCode();
    
    // Machine position updates
    void UpdateToolPosition(float x, float y, float z);
    void ClearToolPosition();
    
    // View controls
    void ZoomToFit();
    void ZoomIn();
    void ZoomOut();
    void ResetView();
    void SetShowGrid(bool show);
    void SetShowOrigin(bool show);
    void SetShowToolPath(bool show);
    void SetShowCurrentPosition(bool show);
    
    // Machine workspace settings
    void SetWorkspaceSize(float width, float height, float depth);

private:
    // Event handlers
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    
    // G-code parsing
    void ParseGCode(const wxString& gcode);
    void ParseGCodeLine(const wxString& line, float& currentX, float& currentY, float& currentZ, bool& isRapid);
    
    // Drawing methods
    void DrawBackground(wxGraphicsContext* gc);
    void DrawGrid(wxGraphicsContext* gc);
    void DrawOrigin(wxGraphicsContext* gc);
    void DrawWorkspaceBounds(wxGraphicsContext* gc);
    void DrawGCodePath(wxGraphicsContext* gc);
    void DrawCurrentPosition(wxGraphicsContext* gc);
    void DrawCoordinateSystem(wxGraphicsContext* gc);
    void DrawStatusInfo(wxGraphicsContext* gc);
    
    // Coordinate transformation
    wxPoint2DDouble WorldToScreen(float x, float y);
    wxPoint2DDouble ScreenToWorld(wxPoint screenPoint);
    void UpdateTransform();
    
    // Data members
    std::vector<GCodeLine> m_gcodeLines;
    ToolPosition m_toolPosition;
    
    // View settings
    float m_viewOffsetX, m_viewOffsetY;
    float m_zoomFactor;
    bool m_showGrid;
    bool m_showOrigin;
    bool m_showToolPath;
    bool m_showCurrentPosition;
    bool m_showWorkspaceBounds;
    
    // Workspace dimensions
    float m_workspaceWidth, m_workspaceHeight, m_workspaceDepth;
    
    // Mouse interaction
    bool m_dragging;
    wxPoint m_lastMousePos;
    
    // G-code bounds
    float m_minX, m_maxX, m_minY, m_maxY, m_minZ, m_maxZ;
    bool m_boundsValid;
    
    // Current file info
    wxString m_currentFilename;
    int m_totalLines;
    
    wxDECLARE_EVENT_TABLE();
};
