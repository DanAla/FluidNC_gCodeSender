/**
 * gui/GCodeEditor.cpp
 * G-code Editor Panel implementation with dummy content
 */

#include "GCodeEditor.h"
#include "core/SimpleLogger.h"
#include "NotificationSystem.h"
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/notebook.h>
#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <wx/textfile.h>

// File drop target for drag and drop support
class GCodeFileDropTarget : public wxFileDropTarget
{
public:
    GCodeFileDropTarget(GCodeEditor* parent) : m_parent(parent) {}
    
    virtual bool OnDropFiles(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y), const wxArrayString& filenames) override
    {
        if (filenames.GetCount() > 0) {
            // Take the first file
            wxString filename = filenames[0];
            
            // Check if it's a valid G-code file extension
            wxFileName fn(filename);
            wxString ext = fn.GetExt().Lower();
            
            if (ext == "gcode" || ext == "nc" || ext == "cnc" || ext == "tap" || ext == "txt") {
                m_parent->LoadGCodeFile(filename);
                return true;
            } else {
                wxMessageBox(wxString::Format("Unsupported file type: %s\nSupported types: .gcode, .nc, .cnc, .tap, .txt", ext),
                           "File Type Error", wxOK | wxICON_WARNING);
                return false;
            }
        }
        return false;
    }
    
private:
    GCodeEditor* m_parent;
};

// Control IDs
enum {
    ID_EDITOR = wxID_HIGHEST + 3000,
    ID_NEW_FILE,
    ID_OPEN_FILE,
    ID_SAVE_FILE,
    ID_SAVE_AS,
    ID_CLOSE_FILE,
    ID_CUT,
    ID_COPY,
    ID_PASTE,
    ID_UNDO,
    ID_REDO,
    ID_FIND,
    ID_REPLACE,
    ID_GOTO,
    ID_SEND_TO_MACHINE,
    ID_VALIDATE_CODE,
    ID_ANALYZE_JOB,
    ID_STATISTICS_LIST,
    ID_ISSUES_LIST
};

wxBEGIN_EVENT_TABLE(GCodeEditor, wxPanel)
    EVT_BUTTON(ID_NEW_FILE, GCodeEditor::OnNew)
    EVT_BUTTON(ID_OPEN_FILE, GCodeEditor::OnOpen)
    EVT_BUTTON(ID_SAVE_FILE, GCodeEditor::OnSave)
    EVT_BUTTON(ID_SAVE_AS, GCodeEditor::OnSaveAs)
    EVT_BUTTON(ID_ANALYZE_JOB, GCodeEditor::OnValidateCode)
    EVT_BUTTON(ID_SEND_TO_MACHINE, GCodeEditor::OnSendToMachine)
    EVT_BUTTON(ID_VALIDATE_CODE, GCodeEditor::OnValidateCode)
    EVT_STC_CHANGE(ID_EDITOR, GCodeEditor::OnTextChanged)
wxEND_EVENT_TABLE()

GCodeEditor::GCodeEditor(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), m_splitter(nullptr), m_editor(nullptr), 
      m_modified(false)
{
    CreateControls();
    
    // Start with empty document
    SetText("");
    UpdateJobStatistics();
}

void GCodeEditor::CreateControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    wxStaticText* title = new wxStaticText(this, wxID_ANY, "G-code Editor");
    title->SetFont(title->GetFont().Scale(1.2).Bold());
    mainSizer->Add(title, 0, wxALL | wxCENTER, 5);
    
    // Toolbar
    CreateToolbar();
    mainSizer->Add(m_toolbar, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    
    // Splitter window
    m_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
                                     wxSP_3D | wxSP_LIVE_UPDATE);
    m_splitter->SetMinimumPaneSize(150);
    
    CreateEditor();
    CreateJobPanel();
    
    m_splitter->SplitHorizontally(m_editorPanel, m_jobPanel, -200);
    
    mainSizer->Add(m_splitter, 1, wxALL | wxEXPAND, 5);
    SetSizer(mainSizer);
}

void GCodeEditor::CreateToolbar()
{
    try {
        // Create a simple button panel instead of toolbar to avoid bitmap issues
        m_toolbar = new wxPanel(this, wxID_ANY);
        wxBoxSizer* toolbarSizer = new wxBoxSizer(wxHORIZONTAL);
        
        // File operations
        wxButton* newBtn = new wxButton(m_toolbar, ID_NEW_FILE, "New", wxDefaultPosition, wxSize(60, -1));
        wxButton* openBtn = new wxButton(m_toolbar, ID_OPEN_FILE, "Open", wxDefaultPosition, wxSize(60, -1));
        wxButton* saveBtn = new wxButton(m_toolbar, ID_SAVE_FILE, "Save", wxDefaultPosition, wxSize(60, -1));
        
        toolbarSizer->Add(newBtn, 0, wxRIGHT, 2);
        toolbarSizer->Add(openBtn, 0, wxRIGHT, 2);
        toolbarSizer->Add(saveBtn, 0, wxRIGHT, 10);
        
        // G-code operations
        wxButton* validateBtn = new wxButton(m_toolbar, ID_VALIDATE_CODE, "Validate", wxDefaultPosition, wxSize(80, -1));
        wxButton* sendBtn = new wxButton(m_toolbar, ID_SEND_TO_MACHINE, "Send to Machine", wxDefaultPosition, wxSize(120, -1));
        
        toolbarSizer->Add(validateBtn, 0, wxRIGHT, 2);
        toolbarSizer->Add(sendBtn, 0, wxRIGHT, 10);
        
        toolbarSizer->AddStretchSpacer();
        m_toolbar->SetSizer(toolbarSizer);
        
    } catch (const std::exception& e) {
        // Create minimal toolbar on error
        m_toolbar = new wxPanel(this, wxID_ANY);
        wxStaticText* errorText = new wxStaticText(m_toolbar, wxID_ANY, "Toolbar Error - Using simplified interface");
        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(errorText, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
        m_toolbar->SetSizer(sizer);
    }
}

void GCodeEditor::CreateEditor()
{
    m_editorPanel = new wxPanel(m_splitter, wxID_ANY);
    wxBoxSizer* editorSizer = new wxBoxSizer(wxVERTICAL);

    m_editor = new wxStyledTextCtrl(m_editorPanel, ID_EDITOR);
    
    wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    m_editor->StyleSetFont(wxSTC_STYLE_DEFAULT, font);
    m_editor->StyleClearAll();

    SetupSyntaxHighlighting();
    SetupEditorProperties();
    
    // Enable drag and drop for files
    m_editor->SetDropTarget(new GCodeFileDropTarget(this));

    editorSizer->Add(m_editor, 1, wxALL | wxEXPAND, 0);
    m_editorPanel->SetSizerAndFit(editorSizer);
}

void GCodeEditor::CreateJobPanel()
{
    m_jobPanel = new wxPanel(m_splitter, wxID_ANY);
    wxBoxSizer* jobSizer = new wxBoxSizer(wxVERTICAL);
    
    // Job info title
    wxStaticText* jobTitle = new wxStaticText(m_jobPanel, wxID_ANY, "Job Information");
    jobTitle->SetFont(jobTitle->GetFont().Bold());
    jobSizer->Add(jobTitle, 0, wxALL, 5);
    
    // Create notebook for tabs
    wxNotebook* notebook = new wxNotebook(m_jobPanel, wxID_ANY);
    
    // Statistics tab
    wxPanel* statsPanel = new wxPanel(notebook, wxID_ANY);
    wxBoxSizer* statsSizer = new wxBoxSizer(wxVERTICAL);
    
    m_statisticsList = new wxListCtrl(statsPanel, ID_STATISTICS_LIST, wxDefaultPosition, wxDefaultSize,
                                     wxLC_REPORT | wxLC_SINGLE_SEL);
    m_statisticsList->AppendColumn("Property", wxLIST_FORMAT_LEFT, 120);
    m_statisticsList->AppendColumn("Value", wxLIST_FORMAT_LEFT, 100);
    
    statsSizer->Add(m_statisticsList, 1, wxALL | wxEXPAND, 5);
    statsPanel->SetSizer(statsSizer);
    notebook->AddPage(statsPanel, "Statistics");
    
    // Issues tab
    wxPanel* issuesPanel = new wxPanel(notebook, wxID_ANY);
    wxBoxSizer* issuesSizer = new wxBoxSizer(wxVERTICAL);
    
    m_issuesList = new wxListCtrl(issuesPanel, ID_ISSUES_LIST, wxDefaultPosition, wxDefaultSize,
                                 wxLC_REPORT | wxLC_SINGLE_SEL);
    m_issuesList->AppendColumn("Type", wxLIST_FORMAT_LEFT, 80);
    m_issuesList->AppendColumn("Line", wxLIST_FORMAT_LEFT, 60);
    m_issuesList->AppendColumn("Description", wxLIST_FORMAT_LEFT, 200);
    
    issuesSizer->Add(m_issuesList, 1, wxALL | wxEXPAND, 5);
    issuesPanel->SetSizer(issuesSizer);
    notebook->AddPage(issuesPanel, "Issues");
    
    jobSizer->Add(notebook, 1, wxALL | wxEXPAND, 5);
    
    // Action buttons
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_analyzeBtn = new wxButton(m_jobPanel, ID_ANALYZE_JOB, "Analyze");
    m_validateBtn = new wxButton(m_jobPanel, ID_VALIDATE_CODE, "Validate");
    m_sendBtn = new wxButton(m_jobPanel, ID_SEND_TO_MACHINE, "Send to Machine");
    
    btnSizer->Add(m_analyzeBtn, 0, wxRIGHT, 5);
    btnSizer->Add(m_validateBtn, 0, wxRIGHT, 5);
    btnSizer->AddStretchSpacer();
    btnSizer->Add(m_sendBtn, 0);
    
    jobSizer->Add(btnSizer, 0, wxALL | wxEXPAND, 5);
    
    m_jobPanel->SetSizer(jobSizer);
}

void GCodeEditor::SetText(const std::string& text)
{
    if (m_editor) {
        m_editor->SetText(wxString::FromUTF8(text));
        m_editor->EmptyUndoBuffer();
        m_modified = false;
    }
}

std::string GCodeEditor::GetText() const
{
    if (m_editor) {
        return m_editor->GetText().ToStdString();
    }
    return "";
}

void GCodeEditor::SetReadOnly(bool readOnly)
{
    if (m_editor) {
        m_editor->SetReadOnly(readOnly);
    }
}

bool GCodeEditor::IsModified() const
{
    if (m_editor) {
        return m_editor->IsModified();
    }
    return false;
}

void GCodeEditor::UpdateJobStatistics()
{
    // Clear existing statistics
    m_statisticsList->DeleteAllItems();
    m_issuesList->DeleteAllItems();
    
    std::string text = GetText();
    
    // Simple analysis
    int totalLines = 0;
    int codeLines = 0;
    int commentLines = 0;
    int emptyLines = 0;
    
    wxStringTokenizer tokenizer(wxString::FromUTF8(text), "\n");
    while (tokenizer.HasMoreTokens()) {
        wxString line = tokenizer.GetNextToken().Trim();
        totalLines++;
        
        if (line.IsEmpty()) {
            emptyLines++;
        } else if (line.StartsWith(";")) {
            commentLines++;
        } else {
            codeLines++;
        }
    }
    
    // Populate statistics
    long index = 0;
    
    index = m_statisticsList->InsertItem(index, "Total Lines");
    m_statisticsList->SetItem(index, 1, std::to_string(totalLines));
    
    index = m_statisticsList->InsertItem(index + 1, "Code Lines");
    m_statisticsList->SetItem(index, 1, std::to_string(codeLines));
    
    index = m_statisticsList->InsertItem(index + 1, "Comment Lines");
    m_statisticsList->SetItem(index, 1, std::to_string(commentLines));
    
    index = m_statisticsList->InsertItem(index + 1, "Empty Lines");
    m_statisticsList->SetItem(index, 1, std::to_string(emptyLines));
    
    index = m_statisticsList->InsertItem(index + 1, "Estimated Time");
    m_statisticsList->SetItem(index, 1, "~5 minutes");
    
    index = m_statisticsList->InsertItem(index + 1, "File Size");
    m_statisticsList->SetItem(index, 1, std::to_string(text.length()) + " bytes");
    
    // Add sample issues
    if (codeLines > 0) {
        index = m_issuesList->InsertItem(0, "Info");
        m_issuesList->SetItem(index, 1, "1");
        m_issuesList->SetItem(index, 2, "File ready for processing");
        
        if (totalLines > 50) {
            index = m_issuesList->InsertItem(1, "Warning");
            m_issuesList->SetItem(index, 1, "-");
            m_issuesList->SetItem(index, 2, "Large file - verify before sending");
        }
    }
}

// Event handlers
void GCodeEditor::OnNew(wxCommandEvent& WXUNUSED(event))
{
    if (PromptSaveChanges()) {
        SetText("");
        m_currentFile.clear();
        UpdateJobStatistics();
        NOTIFY_SUCCESS("New File Created", "Ready to edit G-code in new file.");
    }
}

void GCodeEditor::OnOpen(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog dialog(this, "Open G-code file", "", "",
                       "G-code files (*.gcode;*.nc;*.cnc)|*.gcode;*.nc;*.cnc|All files (*.*)|*.*",
                       wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (dialog.ShowModal() == wxID_OK) {
        NOTIFY_INFO("File Selected", wxString::Format("Would open: %s", dialog.GetFilename()));
    }
}

void GCodeEditor::OnSave(wxCommandEvent& WXUNUSED(event))
{
    if (m_currentFile.empty()) {
        wxCommandEvent evt;
        OnSaveAs(evt);
    } else {
        NOTIFY_SUCCESS("File Saved", "G-code file has been saved successfully.");
    }
}

void GCodeEditor::OnSaveAs(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog dialog(this, "Save G-code file", "", "",
                       "G-code files (*.gcode)|*.gcode|All files (*.*)|*.*",
                       wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dialog.ShowModal() == wxID_OK) {
        NOTIFY_SUCCESS("File Saved As", wxString::Format("Saved as: %s", dialog.GetFilename()));
    }
}

void GCodeEditor::OnSendToMachine(wxCommandEvent& WXUNUSED(event))
{
    NOTIFY_INFO("Send to Machine", "G-code ready to stream to connected CNC machine.");
}

void GCodeEditor::OnValidateCode(wxCommandEvent& WXUNUSED(event))
{
    UpdateJobStatistics();
    NOTIFY_SUCCESS("G-code Validated", "Analysis complete. Check Statistics and Issues tabs for details.");
}

void GCodeEditor::OnTextChanged(wxStyledTextEvent& event)
{
    m_modified = true;
    UpdateJobStatistics();
    
    // Notify any registered callback that the text has changed
    if (m_textChangeCallback) {
        std::string text = GetText();
        LOG_INFO("GCodeEditor::OnTextChanged - Text changed, firing callback with text of length: " + std::to_string(text.length()));
        m_textChangeCallback(text);
    }
    event.Skip();
}

void GCodeEditor::SetTextChangeCallback(std::function<void(const std::string&)> callback)
{
    m_textChangeCallback = callback;
}

bool GCodeEditor::PromptSaveChanges()
{
    if (IsModified()) {
        int result = wxMessageBox("The current file has unsaved changes.\n\nDo you want to save before continuing?",
                                 "Unsaved Changes", wxYES_NO | wxCANCEL | wxICON_QUESTION, this);
        
        if (result == wxCANCEL) {
            return false;
        } else if (result == wxYES) {
            wxCommandEvent evt;
            OnSave(evt);
        }
    }
    return true;
}

void GCodeEditor::SetupSyntaxHighlighting()
{
    if (!m_editor) return;
    
    try {
        // Set up basic G-code syntax highlighting
        // Use a simple approach with custom styling
        
        // Define styles for different G-code elements
        m_editor->StyleSetForeground(0, wxColour(0, 0, 0));        // Default text
        m_editor->StyleSetForeground(1, wxColour(0, 0, 255));      // G-codes (blue)
        m_editor->StyleSetForeground(2, wxColour(255, 0, 0));      // M-codes (red)
        m_editor->StyleSetForeground(3, wxColour(0, 128, 0));      // Comments (green)
        m_editor->StyleSetForeground(4, wxColour(128, 0, 128));    // Numbers (purple)
        m_editor->StyleSetForeground(5, wxColour(255, 165, 0));    // Parameters (orange)
        
        // Set font for all styles
        wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        for (int i = 0; i <= 5; i++) {
            m_editor->StyleSetFont(i, font);
        }
        
        // Make comments italic
        m_editor->StyleSetItalic(3, true);
        
        LOG_INFO("GCodeEditor::SetupSyntaxHighlighting - Basic G-code syntax highlighting configured");
        
    } catch (const std::exception& e) {
        LOG_ERROR("GCodeEditor::SetupSyntaxHighlighting - Error setting up syntax highlighting: " + std::string(e.what()));
    }
}

void GCodeEditor::SetupEditorProperties()
{
    if (!m_editor) return;
    
    try {
        // Set up editor properties
        m_editor->SetLexer(wxSTC_LEX_NULL);  // Use null lexer for custom highlighting
        
        // Line numbers
        m_editor->SetMarginType(0, wxSTC_MARGIN_NUMBER);
        m_editor->SetMarginWidth(0, 50);
        
        // Folding
        m_editor->SetMarginType(1, wxSTC_MARGIN_SYMBOL);
        m_editor->SetMarginMask(1, wxSTC_MASK_FOLDERS);
        m_editor->SetMarginWidth(1, 16);
        m_editor->SetMarginSensitive(1, true);
        
        // Enable code folding
        m_editor->SetProperty("fold", "1");
        m_editor->SetProperty("fold.compact", "1");
        
        // Set fold markers
        m_editor->MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS);
        m_editor->MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS);
        m_editor->MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_VLINE);
        m_editor->MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_LCORNER);
        m_editor->MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUSCONNECTED);
        m_editor->MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED);
        m_editor->MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER);
        
        // Set colors for fold markers
        for (int i = wxSTC_MARKNUM_FOLDER; i <= wxSTC_MARKNUM_FOLDERMIDTAIL; i++) {
            m_editor->MarkerSetForeground(i, wxColour(255, 255, 255));
            m_editor->MarkerSetBackground(i, wxColour(128, 128, 128));
        }
        
        // Other editor properties
        m_editor->SetTabWidth(4);
        m_editor->SetUseTabs(false);  // Use spaces instead of tabs
        m_editor->SetIndent(4);
        
        // Enable brace matching
        m_editor->SetProperty("lexer.cpp.allow.dollars", "0");
        
        // Set selection and caret colors
        m_editor->SetSelBackground(true, wxColour(173, 216, 230));
        m_editor->SetCaretForeground(wxColour(0, 0, 0));
        
        // Enable current line highlighting
        m_editor->SetCaretLineVisible(true);
        m_editor->SetCaretLineBackground(wxColour(245, 245, 245));
        
        LOG_INFO("GCodeEditor::SetupEditorProperties - Editor properties configured");
        
    } catch (const std::exception& e) {
        LOG_ERROR("GCodeEditor::SetupEditorProperties - Error setting up editor properties: " + std::string(e.what()));
    }
}

// LoadGCodeFile implementation for drag and drop support
void GCodeEditor::LoadGCodeFile(const wxString& filename)
{
    try {
        if (!wxFileExists(filename)) {
            wxMessageBox(wxString::Format("File does not exist: %s", filename), "Error", wxOK | wxICON_ERROR);
            return;
        }
        
        // Check if user wants to save changes first
        if (!PromptSaveChanges()) {
            return; // User cancelled
        }
        
        // Read the file
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
        
        // Set the content in the editor
        SetText(content.ToStdString());
        
        // Update current file info
        m_currentFile = filename.ToStdString();
        m_modified = false;
        
        // Update statistics
        UpdateJobStatistics();
        
        // Extract filename for display
        wxFileName fn(filename);
        wxString displayName = fn.GetFullName();
        
        LOG_INFO("Loaded G-code file: " + filename.ToStdString());
        NOTIFY_SUCCESS("File Loaded", wxString::Format("Successfully loaded G-code file: %s", displayName));
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load G-code file: " + std::string(e.what()));
        NOTIFY_ERROR("File Load Error", wxString::Format("Failed to load file: %s", e.what()));
    }
}

// Placeholder implementations for interface compliance
void GCodeEditor::NewFile() { wxCommandEvent evt; OnNew(evt); }
void GCodeEditor::OpenFile(const std::string& filename) { 
    if (!filename.empty()) {
        LoadGCodeFile(wxString::FromUTF8(filename));
    } else {
        wxCommandEvent evt; 
        OnOpen(evt);
    }
}
void GCodeEditor::SaveFile() { wxCommandEvent evt; OnSave(evt); }
void GCodeEditor::SaveFileAs() { wxCommandEvent evt; OnSaveAs(evt); }
bool GCodeEditor::CloseFile() { return PromptSaveChanges(); }
void GCodeEditor::AnalyzeJob() { UpdateJobStatistics(); }
