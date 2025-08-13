/**
 * gui/MachineVisualizationPanel.cpp
 * Machine visualization panel implementation
 */

#include "MachineVisualizationPanel.h"
#include "core/SimpleLogger.h"
#include "core/GCodeParser.h"
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/msgdlg.h>
#include <wx/tokenzr.h>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <map>

// Event table
wxBEGIN_EVENT_TABLE(MachineVisualizationPanel, wxPanel)
    EVT_PAINT(MachineVisualizationPanel::OnPaint)
    EVT_SIZE(MachineVisualizationPanel::OnSize)
    EVT_MOUSEWHEEL(MachineVisualizationPanel::OnMouseWheel)
    EVT_LEFT_DOWN(MachineVisualizationPanel::OnMouseDown)
    EVT_MOTION(MachineVisualizationPanel::OnMouseMove)
    EVT_LEFT_UP(MachineVisualizationPanel::OnMouseUp)
    EVT_KEY_DOWN(MachineVisualizationPanel::OnKeyDown)
wxEND_EVENT_TABLE()

MachineVisualizationPanel::MachineVisualizationPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
    , m_viewOffsetX(0.0f)
    , m_viewOffsetY(0.0f)
    , m_zoomFactor(1.0f)
    , m_showGrid(true)
    , m_showOrigin(true)
    , m_showToolPath(true)
    , m_showCurrentPosition(true)
    , m_showWorkspaceBounds(true)
    , m_workspaceWidth(300.0f)
    , m_workspaceHeight(200.0f)
    , m_workspaceDepth(100.0f)
    , m_dragging(false)
    , m_minX(0), m_maxX(0), m_minY(0), m_maxY(0), m_minZ(0), m_maxZ(0)
    , m_boundsValid(false)
    , m_totalLines(0)
{
    // Initialize tool position as invalid
    m_toolPosition.isValid = false;
    
    // Set background color
    SetBackgroundColour(wxColour(240, 240, 240));
    
    // Enable key events
    SetCanFocus(true);
    
    LOG_INFO("Machine Visualization Panel created");
}

MachineVisualizationPanel::~MachineVisualizationPanel()
{
    LOG_INFO("Machine Visualization Panel destroyed");
}

void MachineVisualizationPanel::LoadGCodeFile(const wxString& filename)
{
    if (!wxFileExists(filename)) {
        wxMessageBox(wxString::Format("File does not exist: %s", filename), "Error", wxOK | wxICON_ERROR);
        return;
    }
    
    wxTextFile file;
    if (!file.Open(filename)) {
        wxMessageBox(wxString::Format("Cannot open file: %s", filename), "Error", wxOK | wxICON_ERROR);
        return;
    }
    
    wxString content;
    for (size_t i = 0; i < file.GetLineCount(); i++) {
        content += file[i] + "\n";
    }
    
    file.Close();
    
    m_currentFilename = wxFileName(filename).GetFullName();
    SetGCodeContent(content);
    
    LOG_INFO("Loaded G-code file: " + filename.ToStdString());
}

void MachineVisualizationPanel::SetGCodeContent(const wxString& gcode)
{
    LOG_INFO(wxString::Format("SetGCodeContent called with gcode of length %zu", gcode.length()).ToStdString());
    ClearGCode();
    ParseGCode(gcode);
    LOG_INFO(wxString::Format("Parsing complete. %zu path segments generated.", m_gcodeLines.size()).ToStdString());
    ZoomToFit();
    Refresh();
}

void MachineVisualizationPanel::ClearGCode()
{
    m_gcodeLines.clear();
    m_boundsValid = false;
    m_totalLines = 0;
    m_currentFilename.Clear();
    Refresh();
}

void MachineVisualizationPanel::UpdateToolPosition(float x, float y, float z)
{
    m_toolPosition.x = x;
    m_toolPosition.y = y;
    m_toolPosition.z = z;
    m_toolPosition.isValid = true;
    m_toolPosition.lastUpdate = wxDateTime::Now();
    
    Refresh();
}

void MachineVisualizationPanel::ClearToolPosition()
{
    m_toolPosition.isValid = false;
    Refresh();
}


void MachineVisualizationPanel::UpdateBounds(float x, float y)
{
    if (!m_boundsValid) {
        m_minX = m_maxX = x;
        m_minY = m_maxY = y;
        m_boundsValid = true;
    } else {
        m_minX = std::min(m_minX, x);
        m_maxX = std::max(m_maxX, x);
        m_minY = std::min(m_minY, y);
        m_maxY = std::max(m_maxY, y);
    }
}

void MachineVisualizationPanel::AddLineSegment(float x, float y, bool isRapid)
{
    GCodeLine gcLine;
    gcLine.startX = m_currentX;
    gcLine.startY = m_currentY;
    gcLine.startZ = m_currentZ;
    gcLine.endX = x;
    gcLine.endY = y;
    gcLine.endZ = m_currentZ; // Assuming 2D for now
    gcLine.isRapid = isRapid;
    gcLine.color = isRapid ? wxColour(255, 0, 0) : wxColour(0, 100, 255);
    
    m_gcodeLines.push_back(gcLine);
    
    UpdateBounds(m_currentX, m_currentY);
    UpdateBounds(x, y);
    
    m_currentX = x;
    m_currentY = y;
}

void MachineVisualizationPanel::AddArcSegments(float x, float y, float i, float j, bool isClockwise)
{
    float startX = m_currentX;
    float startY = m_currentY;
    float endX = m_isIncrementalMode ? startX + x : x;
    float endY = m_isIncrementalMode ? startY + y : y;

    float centerX = startX + i;
    float centerY = startY + j;

    float radius = sqrt(i * i + j * j);
    float startAngle = atan2(startY - centerY, startX - centerX);
    float endAngle = atan2(endY - centerY, endX - centerX);

    if (isClockwise) { // G2
        if (endAngle >= startAngle) {
            endAngle -= 2 * M_PI;
        }
    } else { // G3
        if (endAngle <= startAngle) {
            endAngle += 2 * M_PI;
        }
    }

    float totalAngle = endAngle - startAngle;
    int numSegments = std::max(1, static_cast<int>(fabs(totalAngle) * radius / 0.5)); // Segments for ~0.5mm chord length

    float angleStep = totalAngle / numSegments;

    float lastX = startX;
    float lastY = startY;

    for (int k = 1; k <= numSegments; ++k) {
        float angle = startAngle + k * angleStep;
        float nextX = centerX + radius * cos(angle);
        float nextY = centerY + radius * sin(angle);

        GCodeLine gcLine;
        gcLine.startX = lastX;
        gcLine.startY = lastY;
        gcLine.startZ = m_currentZ;
        gcLine.endX = nextX;
        gcLine.endY = nextY;
        gcLine.endZ = m_currentZ;
        gcLine.isRapid = false;
        gcLine.color = wxColour(0, 100, 255);
        m_gcodeLines.push_back(gcLine);

        UpdateBounds(lastX, lastY);
        UpdateBounds(nextX, nextY);

        lastX = nextX;
        lastY = nextY;
    }

    m_currentX = endX;
    m_currentY = endY;
}


void MachineVisualizationPanel::ParseGCode(const wxString& gcode)
{
    LOG_INFO("ParseGCode started with comprehensive parser.");
    
    // Clear previous visualization data
    m_gcodeLines.clear();
    m_boundsValid = false;
    
    // Create parser instance
    GCodeParser parser;
    
    // Set up callbacks for real-time updates
    parser.setProgressCallback([this](int currentLine, int totalLines) {
        // Could update progress bar if we had one
        // For now, just log occasionally
        if (currentLine % 100 == 0) {
            LOG_INFO(wxString::Format("Parsing progress: %d/%d lines", currentLine, totalLines).ToStdString());
        }
    });
    
    parser.setErrorCallback([this](const ParseError& error) {
        LOG_ERROR(wxString::Format("Parse error at line %d: %s", 
                                  error.lineNumber, error.message).ToStdString());
    });
    
    // Enable comprehensive parsing features
    parser.enableStatistics(true);
    parser.enableToolpathGeneration(true);
    parser.setStrictMode(false); // Be lenient with non-standard G-code
    
    // Parse the G-code
    std::string gcodeStd = gcode.ToStdString();
    bool success = parser.parseString(gcodeStd);
    
    if (!success) {
        LOG_ERROR("G-code parsing failed with errors");
        const auto& errors = parser.getErrors();
        for (const auto& error : errors) {
            LOG_ERROR(wxString::Format("Line %d: %s", error.lineNumber, error.message).ToStdString());
        }
    }
    
    // Get parsing results
    const auto& toolpath = parser.getToolpath();
    const auto& statistics = parser.getStatistics();
    
    // Convert toolpath segments to visualization lines
    for (const auto& segment : toolpath) {
        GCodeLine gcLine;
        gcLine.startX = static_cast<float>(segment.start.x);
        gcLine.startY = static_cast<float>(segment.start.y);
        gcLine.startZ = static_cast<float>(segment.start.z);
        gcLine.endX = static_cast<float>(segment.end.x);
        gcLine.endY = static_cast<float>(segment.end.y);
        gcLine.endZ = static_cast<float>(segment.end.z);
        
        // Set color and style based on segment type
        switch (segment.type) {
            case ToolpathSegment::RAPID:
                gcLine.isRapid = true;
                gcLine.color = wxColour(255, 0, 0); // Red for rapid moves
                break;
            case ToolpathSegment::LINEAR:
                gcLine.isRapid = false;
                gcLine.color = wxColour(0, 100, 255); // Blue for cutting moves
                break;
            case ToolpathSegment::ARC_CW:
            case ToolpathSegment::ARC_CCW:
                gcLine.isRapid = false;
                gcLine.color = wxColour(0, 150, 0); // Green for arcs
                break;
            case ToolpathSegment::DRILL_CYCLE:
                gcLine.isRapid = false;
                gcLine.color = wxColour(255, 165, 0); // Orange for drilling
                break;
        }
        
        m_gcodeLines.push_back(gcLine);
        
        // Update bounds
        UpdateBounds(gcLine.startX, gcLine.startY);
        UpdateBounds(gcLine.endX, gcLine.endY);
    }
    
    // Update statistics
    m_totalLines = statistics.totalLines;
    
    // Apply bounds from parser if valid
    if (statistics.boundsValid) {
        m_minX = static_cast<float>(statistics.minBounds.x);
        m_maxX = static_cast<float>(statistics.maxBounds.x);
        m_minY = static_cast<float>(statistics.minBounds.y);
        m_maxY = static_cast<float>(statistics.maxBounds.y);
        m_minZ = static_cast<float>(statistics.minBounds.z);
        m_maxZ = static_cast<float>(statistics.maxBounds.z);
        m_boundsValid = true;
    }
    
    // Log comprehensive statistics
    LOG_INFO(wxString::Format("G-code parsing completed: %d total lines, %d command lines, %d segments", 
                             statistics.totalLines, statistics.commandLines, static_cast<int>(toolpath.size())).ToStdString());
    LOG_INFO(wxString::Format("Movement statistics: %d rapid moves, %d linear moves, %d arc moves, %d tool changes", 
                             statistics.rapidMoves, statistics.linearMoves, statistics.arcMoves, statistics.toolChanges).ToStdString());
    
    if (statistics.boundsValid) {
        LOG_INFO(wxString::Format("G-code bounds: X(%.2f to %.2f), Y(%.2f to %.2f), Z(%.2f to %.2f)", 
                                 statistics.minBounds.x, statistics.maxBounds.x,
                                 statistics.minBounds.y, statistics.maxBounds.y,
                                 statistics.minBounds.z, statistics.maxBounds.z).ToStdString());
    }
    
    if (statistics.estimatedTime > 0) {
        LOG_INFO(wxString::Format("Estimated machining time: %.2f minutes", statistics.estimatedTime).ToStdString());
    }
    
    if (statistics.errorLines > 0) {
        LOG_WARNING(wxString::Format("Parsing completed with %d error lines", statistics.errorLines).ToStdString());
    }
}

void MachineVisualizationPanel::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    
    // Create graphics context
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc) return;
    
    try {
        // Clear background
        DrawBackground(gc);
        
        // Set up coordinate system (flip Y axis for standard CNC orientation)
        wxSize clientSize = GetClientSize();
        gc->Translate(clientSize.x / 2.0 + m_viewOffsetX, clientSize.y / 2.0 - m_viewOffsetY);
        gc->Scale(m_zoomFactor, -m_zoomFactor); // Flip Y axis
        
        // Draw components in order
        if (m_showWorkspaceBounds) DrawWorkspaceBounds(gc);
        if (m_showGrid) DrawGrid(gc);
        if (m_showOrigin) DrawOrigin(gc);
        if (m_showToolPath) DrawGCodePath(gc);
        if (m_showCurrentPosition) DrawCurrentPosition(gc);
        
        // Reset transformation for status info
        gc->ResetClip();
        gc->SetTransform(gc->CreateMatrix());
        DrawStatusInfo(gc);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Paint error: " + std::string(e.what()));
    }
    
    delete gc;
}

void MachineVisualizationPanel::DrawBackground(wxGraphicsContext* gc)
{
    wxSize size = GetClientSize();
    gc->SetBrush(wxBrush(GetBackgroundColour()));
    gc->DrawRectangle(0, 0, size.x, size.y);
}

void MachineVisualizationPanel::DrawGrid(wxGraphicsContext* gc)
{
    gc->SetPen(wxPen(wxColour(200, 200, 200), 1));
    
    // Grid spacing based on zoom level
    float gridSpacing = 10.0f;
    if (m_zoomFactor < 0.1f) gridSpacing = 100.0f;
    else if (m_zoomFactor < 0.5f) gridSpacing = 50.0f;
    else if (m_zoomFactor > 5.0f) gridSpacing = 5.0f;
    else if (m_zoomFactor > 10.0f) gridSpacing = 1.0f;
    
    // Calculate visible area
    wxSize clientSize = GetClientSize();
    float halfWidth = (clientSize.x / 2.0f) / m_zoomFactor;
    float halfHeight = (clientSize.y / 2.0f) / m_zoomFactor;
    
    float left = -halfWidth - m_viewOffsetX / m_zoomFactor;
    float right = halfWidth - m_viewOffsetX / m_zoomFactor;
    float top = halfHeight + m_viewOffsetY / m_zoomFactor;
    float bottom = -halfHeight + m_viewOffsetY / m_zoomFactor;
    
    // Draw vertical lines
    float start = floor(left / gridSpacing) * gridSpacing;
    for (float x = start; x <= right; x += gridSpacing) {
        gc->StrokeLine(x, bottom, x, top);
    }
    
    // Draw horizontal lines
    start = floor(bottom / gridSpacing) * gridSpacing;
    for (float y = start; y <= top; y += gridSpacing) {
        gc->StrokeLine(left, y, right, y);
    }
}

void MachineVisualizationPanel::DrawOrigin(wxGraphicsContext* gc)
{
    gc->SetPen(wxPen(wxColour(0, 0, 0), 2));
    
    // X axis - red
    gc->SetPen(wxPen(wxColour(255, 0, 0), 2));
    gc->StrokeLine(0, 0, 20 / m_zoomFactor, 0);
    
    // Y axis - green
    gc->SetPen(wxPen(wxColour(0, 255, 0), 2));
    gc->StrokeLine(0, 0, 0, 20 / m_zoomFactor);
    
    // Origin point
    gc->SetBrush(wxBrush(wxColour(0, 0, 0)));
    float radius = 3.0f / m_zoomFactor;
    gc->DrawEllipse(-radius, -radius, 2 * radius, 2 * radius);
}

void MachineVisualizationPanel::DrawWorkspaceBounds(wxGraphicsContext* gc)
{
    gc->SetPen(wxPen(wxColour(100, 100, 100), 1, wxPENSTYLE_DOT));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    
    gc->DrawRectangle(0, 0, m_workspaceWidth, m_workspaceHeight);
}

void MachineVisualizationPanel::DrawGCodePath(wxGraphicsContext* gc)
{
    if (m_gcodeLines.empty()) return;
    
    for (const auto& line : m_gcodeLines) {
        gc->SetPen(wxPen(line.color, line.isRapid ? 1 : 2));
        gc->StrokeLine(line.startX, line.startY, line.endX, line.endY);
    }
}

void MachineVisualizationPanel::DrawCurrentPosition(wxGraphicsContext* gc)
{
    if (!m_toolPosition.isValid) return;
    
    // Draw tool position as a crosshair
    gc->SetPen(wxPen(wxColour(255, 100, 0), 3)); // Orange
    
    float size = 10.0f / m_zoomFactor;
    float x = m_toolPosition.x;
    float y = m_toolPosition.y;
    
    // Crosshair
    gc->StrokeLine(x - size, y, x + size, y);
    gc->StrokeLine(x, y - size, x, y + size);
    
    // Circle around position
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawEllipse(x - size/2, y - size/2, size, size);
}

void MachineVisualizationPanel::DrawStatusInfo(wxGraphicsContext* gc)
{
    wxSize size = GetClientSize();
    gc->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL), wxColour(0, 0, 0));
    
    int y = 10;
    const int lineHeight = 15;
    
    // File info
    if (!m_currentFilename.IsEmpty()) {
        gc->DrawText(wxString::Format("File: %s", m_currentFilename), 10, y);
        y += lineHeight;
    }
    
    if (m_totalLines > 0) {
        gc->DrawText(wxString::Format("Lines: %d, Segments: %zu", m_totalLines, m_gcodeLines.size()), 10, y);
        y += lineHeight;
    }
    
    // Tool position
    if (m_toolPosition.isValid) {
        gc->DrawText(wxString::Format("Position: X:%.3f Y:%.3f Z:%.3f", 
                                     m_toolPosition.x, m_toolPosition.y, m_toolPosition.z), 10, y);
        y += lineHeight;
    }
    
    // Bounds info
    if (m_boundsValid) {
        gc->DrawText(wxString::Format("Bounds: X:%.1f-%.1f Y:%.1f-%.1f", 
                                     m_minX, m_maxX, m_minY, m_maxY), 10, y);
        y += lineHeight;
    }
    
    // View info
    gc->DrawText(wxString::Format("Zoom: %.1f%% View: %.1f,%.1f", 
                                 m_zoomFactor * 100, m_viewOffsetX, m_viewOffsetY), 10, y);
}

void MachineVisualizationPanel::ZoomToFit()
{
    if (!m_boundsValid || m_gcodeLines.empty()) {
        m_zoomFactor = 1.0f;
        m_viewOffsetX = m_viewOffsetY = 0.0f;
        Refresh();
        return;
    }
    
    wxSize clientSize = GetClientSize();
    if (clientSize.x <= 0 || clientSize.y <= 0) return;
    
    float width = m_maxX - m_minX;
    float height = m_maxY - m_minY;
    
    if (width <= 0) width = 1.0f;
    if (height <= 0) height = 1.0f;
    
    // Add 10% margin
    width *= 1.1f;
    height *= 1.1f;
    
    float zoomX = clientSize.x / width;
    float zoomY = clientSize.y / height;
    
    m_zoomFactor = std::min(zoomX, zoomY);
    
    // Center on bounds
    m_viewOffsetX = -((m_minX + m_maxX) / 2.0f) * m_zoomFactor;
    m_viewOffsetY = ((m_minY + m_maxY) / 2.0f) * m_zoomFactor;
    
    Refresh();
}

void MachineVisualizationPanel::ZoomIn()
{
    m_zoomFactor *= 1.5f;
    Refresh();
}

void MachineVisualizationPanel::ZoomOut()
{
    m_zoomFactor /= 1.5f;
    if (m_zoomFactor < 0.01f) m_zoomFactor = 0.01f;
    Refresh();
}

void MachineVisualizationPanel::ResetView()
{
    m_zoomFactor = 1.0f;
    m_viewOffsetX = m_viewOffsetY = 0.0f;
    Refresh();
}

void MachineVisualizationPanel::SetShowGrid(bool show)
{
    m_showGrid = show;
    Refresh();
}

void MachineVisualizationPanel::SetShowOrigin(bool show)
{
    m_showOrigin = show;
    Refresh();
}

void MachineVisualizationPanel::SetShowToolPath(bool show)
{
    m_showToolPath = show;
    Refresh();
}

void MachineVisualizationPanel::SetShowCurrentPosition(bool show)
{
    m_showCurrentPosition = show;
    Refresh();
}

void MachineVisualizationPanel::SetWorkspaceSize(float width, float height, float depth)
{
    m_workspaceWidth = width;
    m_workspaceHeight = height;
    m_workspaceDepth = depth;
    Refresh();
}

// Event handlers
void MachineVisualizationPanel::OnSize(wxSizeEvent& event)
{
    Refresh();
    event.Skip();
}

void MachineVisualizationPanel::OnMouseWheel(wxMouseEvent& event)
{
    float delta = event.GetWheelRotation() > 0 ? 1.2f : 1.0f/1.2f;
    m_zoomFactor *= delta;
    if (m_zoomFactor < 0.01f) m_zoomFactor = 0.01f;
    if (m_zoomFactor > 100.0f) m_zoomFactor = 100.0f;
    Refresh();
}

void MachineVisualizationPanel::OnMouseDown(wxMouseEvent& event)
{
    if (event.LeftDown()) {
        m_dragging = true;
        m_lastMousePos = event.GetPosition();
        CaptureMouse();
    }
}

void MachineVisualizationPanel::OnMouseMove(wxMouseEvent& event)
{
    if (m_dragging && event.Dragging()) {
        wxPoint currentPos = event.GetPosition();
        wxPoint delta = currentPos - m_lastMousePos;
        
        m_viewOffsetX += delta.x;
        m_viewOffsetY += delta.y;
        
        m_lastMousePos = currentPos;
        Refresh();
    }
}

void MachineVisualizationPanel::OnMouseUp(wxMouseEvent& event)
{
    if (m_dragging) {
        m_dragging = false;
        ReleaseMouse();
    }
}

void MachineVisualizationPanel::OnKeyDown(wxKeyEvent& event)
{
    switch (event.GetKeyCode()) {
        case 'R':
        case 'r':
            ResetView();
            break;
        case 'F':
        case 'f':
            ZoomToFit();
            break;
        case '+':
        case '=':
            ZoomIn();
            break;
        case '-':
        case '_':
            ZoomOut();
            break;
        default:
            event.Skip();
            break;
    }
}
