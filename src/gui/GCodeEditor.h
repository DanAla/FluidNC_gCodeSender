/**
 * gui/GCodeEditor.h
 * G-code editor panel with syntax highlighting and editing features
 * Provides G-code file editing, syntax highlighting, and job management
 */

#pragma once

#include <wx/wx.h>
#include <wx/stc/stc.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <wx/dnd.h>
#include <vector>
#include <string>
#include <functional>

/**
 * G-code Editor Panel - advanced text editor for G-code files
 * Features:
 * - Syntax highlighting for G-code
 * - Line numbering and code folding
 * - Job statistics and analysis
 * - File operations (open, save, new)
 * - G-code validation and error highlighting
 */
class GCodeEditor : public wxPanel
{
public:
    GCodeEditor(wxWindow* parent);
    
    // File operations
    void NewFile();
    void OpenFile(const std::string& filename = "");
    void SaveFile();
    void SaveFileAs();
    bool CloseFile();
    
    // Editor operations
    void SetText(const std::string& text);
    std::string GetText() const;
    void SetReadOnly(bool readOnly);
    bool IsModified() const;
    
    // Job analysis
    void AnalyzeJob();
    void UpdateJobStatistics();
    
    // Text change callback
    void SetTextChangeCallback(std::function<void(const std::string&)> callback);
    
    // File loading (public for drag and drop)
    void LoadGCodeFile(const wxString& filename);

private:
    // Event handlers
    void OnNew(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnSaveAs(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnCut(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnPaste(wxCommandEvent& event);
    void OnUndo(wxCommandEvent& event);
    void OnRedo(wxCommandEvent& event);
    void OnFind(wxCommandEvent& event);
    void OnReplace(wxCommandEvent& event);
    void OnGoto(wxCommandEvent& event);
    void OnSendToMachine(wxCommandEvent& event);
    void OnValidateCode(wxCommandEvent& event);
    
    // Editor events
    void OnTextChanged(wxStyledTextEvent& event);
    void OnMarginClick(wxStyledTextEvent& event);
    void OnUpdateUI(wxStyledTextEvent& event);
    
    // UI Creation
    void CreateControls();
    void CreateEditor();
    void CreateToolbar();
    void CreateJobPanel();
    void SetupSyntaxHighlighting();
    void SetupEditorProperties();
    
    // File management
    void UpdateTitle();
    void UpdateJobInfo();
    bool PromptSaveChanges();
    
    // Syntax highlighting
    void ConfigureGCodeLexer();
    void SetGCodeKeywords();
    
    // Job analysis
    struct JobStatistics {
        int totalLines;
        int codeLines;
        int commentLines;
        int emptyLines;
        double estimatedTime;
        double totalDistance;
        struct {
            double minX, maxX;
            double minY, maxY;
            double minZ, maxZ;
        } bounds;
        std::vector<std::string> toolsUsed;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
    };
    
    // UI Components
    wxSplitterWindow* m_splitter;
    
    // Editor panel
    wxPanel* m_editorPanel;
    wxPanel* m_toolbar;
    wxStyledTextCtrl* m_editor;
    
    // Job info panel
    wxPanel* m_jobPanel;
    wxListCtrl* m_statisticsList;
    wxListCtrl* m_issuesList;
    wxButton* m_analyzeBtn;
    wxButton* m_sendBtn;
    wxButton* m_validateBtn;
    
    // Current file
    std::string m_currentFile;
    bool m_modified;
    
    // Job data
    JobStatistics m_jobStats;
    
    // Text change callback
    std::function<void(const std::string&)> m_textChangeCallback;
    
    wxDECLARE_EVENT_TABLE();
};
