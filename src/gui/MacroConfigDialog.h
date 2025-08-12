/**
 * gui/MacroConfigDialog.h
 * Dialog for configuring quick command macro buttons
 */

#pragma once

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>
#include <string>

struct MacroDefinition {
    std::string label;
    std::string command;
    std::string description;
    
    MacroDefinition() = default;
    MacroDefinition(const std::string& l, const std::string& c, const std::string& d)
        : label(l), command(c), description(d) {}
};

/**
 * Dialog for configuring macro buttons
 * Allows adding, editing, deleting, and reordering macro commands
 */
class MacroConfigDialog : public wxDialog
{
public:
    MacroConfigDialog(wxWindow* parent, const std::vector<MacroDefinition>& macros);
    ~MacroConfigDialog();
    
    // Get the configured macros
    std::vector<MacroDefinition> GetMacros() const { return m_macros; }
    
    // Check if macros were modified
    bool WereModified() const { return m_modified; }

private:
    // Event handlers
    void OnAdd(wxCommandEvent& event);
    void OnEdit(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnMoveUp(wxCommandEvent& event);
    void OnMoveDown(wxCommandEvent& event);
    void OnReset(wxCommandEvent& event);
    void OnImport(wxCommandEvent& event);
    void OnExport(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnSelectionChanged(wxListEvent& event);
    void OnItemActivated(wxListEvent& event);
    
    // UI creation
    void CreateControls();
    void CreateMacroList();
    void CreateButtons();
    
    // List management
    void UpdateMacroList();
    void UpdateButtonStates();
    int GetSelectedIndex() const;
    void SelectItem(int index);
    
    // Macro operations
    void AddMacro(const MacroDefinition& macro);
    void EditMacro(int index);
    void DeleteMacro(int index);
    void MoveMacroUp(int index);
    void MoveMacroDown(int index);
    void ResetToDefaults();
    
    // File operations
    bool ImportMacros();
    bool ExportMacros();
    
    // Default macros
    std::vector<MacroDefinition> GetDefaultMacros() const;
    
    // UI components
    wxListCtrl* m_macroList;
    wxButton* m_addBtn;
    wxButton* m_editBtn;
    wxButton* m_deleteBtn;
    wxButton* m_moveUpBtn;
    wxButton* m_moveDownBtn;
    wxButton* m_resetBtn;
    wxButton* m_importBtn;
    wxButton* m_exportBtn;
    wxButton* m_okBtn;
    wxButton* m_cancelBtn;
    
    // Data
    std::vector<MacroDefinition> m_macros;
    bool m_modified;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * Dialog for editing a single macro
 */
class MacroEditDialog : public wxDialog
{
public:
    MacroEditDialog(wxWindow* parent, const MacroDefinition& macro, const wxString& title = "Edit Macro");
    ~MacroEditDialog();
    
    MacroDefinition GetMacro() const;

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnLabelChanged(wxCommandEvent& event);
    void OnCommandChanged(wxCommandEvent& event);
    
    void CreateControls();
    void UpdatePreview();
    bool ValidateInput();
    
    // UI components
    wxTextCtrl* m_labelText;
    wxTextCtrl* m_commandText;
    wxTextCtrl* m_descriptionText;
    wxStaticText* m_previewText;
    wxButton* m_okBtn;
    wxButton* m_cancelBtn;
    
    MacroDefinition m_macro;
    
    wxDECLARE_EVENT_TABLE();
};
