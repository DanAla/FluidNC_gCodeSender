/**
 * gui/MacroConfigDialog.cpp
 * Dialog for configuring quick command macro buttons
 */

#include "MacroConfigDialog.h"
#include "NotificationSystem.h"
#include <wx/sizer.h>
#include <wx/filedlg.h>
#include <wx/textfile.h>
#include <wx/msgdlg.h>
#include <wx/statline.h>
#include <algorithm>

// Control IDs
enum {
    ID_MACRO_LIST = wxID_HIGHEST + 6000,
    ID_ADD_MACRO,
    ID_EDIT_MACRO,
    ID_DELETE_MACRO,
    ID_MOVE_UP,
    ID_MOVE_DOWN,
    ID_RESET_MACROS,
    ID_IMPORT_MACROS,
    ID_EXPORT_MACROS,
    
    // Edit dialog IDs
    ID_LABEL_TEXT,
    ID_COMMAND_TEXT,
    ID_DESCRIPTION_TEXT
};

// MacroConfigDialog implementation

wxBEGIN_EVENT_TABLE(MacroConfigDialog, wxDialog)
    EVT_BUTTON(ID_ADD_MACRO, MacroConfigDialog::OnAdd)
    EVT_BUTTON(ID_EDIT_MACRO, MacroConfigDialog::OnEdit)
    EVT_BUTTON(ID_DELETE_MACRO, MacroConfigDialog::OnDelete)
    EVT_BUTTON(ID_MOVE_UP, MacroConfigDialog::OnMoveUp)
    EVT_BUTTON(ID_MOVE_DOWN, MacroConfigDialog::OnMoveDown)
    EVT_BUTTON(ID_RESET_MACROS, MacroConfigDialog::OnReset)
    EVT_BUTTON(ID_IMPORT_MACROS, MacroConfigDialog::OnImport)
    EVT_BUTTON(ID_EXPORT_MACROS, MacroConfigDialog::OnExport)
    EVT_BUTTON(wxID_OK, MacroConfigDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, MacroConfigDialog::OnCancel)
    EVT_LIST_ITEM_SELECTED(ID_MACRO_LIST, MacroConfigDialog::OnSelectionChanged)
    EVT_LIST_ITEM_ACTIVATED(ID_MACRO_LIST, MacroConfigDialog::OnItemActivated)
wxEND_EVENT_TABLE()

MacroConfigDialog::MacroConfigDialog(wxWindow* parent, const std::vector<MacroDefinition>& macros)
    : wxDialog(parent, wxID_ANY, "Configure Quick Commands", wxDefaultPosition, wxSize(600, 450),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_macros(macros)
    , m_modified(false)
{
    CreateControls();
    UpdateMacroList();
    UpdateButtonStates();
    
    // Center the dialog
    CenterOnParent();
}

MacroConfigDialog::~MacroConfigDialog()
{
}

void MacroConfigDialog::CreateControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    wxStaticText* title = new wxStaticText(this, wxID_ANY, "Quick Command Macro Configuration");
    title->SetFont(title->GetFont().Scale(1.2).Bold());
    mainSizer->Add(title, 0, wxALL | wxCENTER, 10);
    
    // Content area
    wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Create macro list
    CreateMacroList();
    contentSizer->Add(m_macroList, 1, wxEXPAND | wxALL, 5);
    
    // Create buttons
    CreateButtons();
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxVERTICAL);
    buttonSizer->Add(m_addBtn, 0, wxEXPAND | wxBOTTOM, 5);
    buttonSizer->Add(m_editBtn, 0, wxEXPAND | wxBOTTOM, 5);
    buttonSizer->Add(m_deleteBtn, 0, wxEXPAND | wxBOTTOM, 10);
    buttonSizer->Add(m_moveUpBtn, 0, wxEXPAND | wxBOTTOM, 5);
    buttonSizer->Add(m_moveDownBtn, 0, wxEXPAND | wxBOTTOM, 10);
    buttonSizer->Add(m_resetBtn, 0, wxEXPAND | wxBOTTOM, 5);
    buttonSizer->Add(m_importBtn, 0, wxEXPAND | wxBOTTOM, 5);
    buttonSizer->Add(m_exportBtn, 0, wxEXPAND);
    
    contentSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(contentSizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);
    
    // Dialog buttons
    mainSizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    
    wxBoxSizer* dlgButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_okBtn = new wxButton(this, wxID_OK, "OK");
    m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    
    dlgButtonSizer->AddStretchSpacer();
    dlgButtonSizer->Add(m_okBtn, 0, wxRIGHT, 5);
    dlgButtonSizer->Add(m_cancelBtn, 0);
    
    mainSizer->Add(dlgButtonSizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(mainSizer);
    
    m_okBtn->SetDefault();
}

void MacroConfigDialog::CreateMacroList()
{
    m_macroList = new wxListCtrl(this, ID_MACRO_LIST, wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
    
    // Add columns
    m_macroList->AppendColumn("Label", wxLIST_FORMAT_LEFT, 80);
    m_macroList->AppendColumn("Command", wxLIST_FORMAT_LEFT, 120);
    m_macroList->AppendColumn("Description", wxLIST_FORMAT_LEFT, 300);
}

void MacroConfigDialog::CreateButtons()
{
    m_addBtn = new wxButton(this, ID_ADD_MACRO, "Add");
    m_editBtn = new wxButton(this, ID_EDIT_MACRO, "Edit");
    m_deleteBtn = new wxButton(this, ID_DELETE_MACRO, "Delete");
    m_moveUpBtn = new wxButton(this, ID_MOVE_UP, "Move Up");
    m_moveDownBtn = new wxButton(this, ID_MOVE_DOWN, "Move Down");
    m_resetBtn = new wxButton(this, ID_RESET_MACROS, "Reset to Defaults");
    m_importBtn = new wxButton(this, ID_IMPORT_MACROS, "Import...");
    m_exportBtn = new wxButton(this, ID_EXPORT_MACROS, "Export...");
    
    // Set tooltips
    m_addBtn->SetToolTip("Add a new macro command");
    m_editBtn->SetToolTip("Edit the selected macro");
    m_deleteBtn->SetToolTip("Delete the selected macro");
    m_moveUpBtn->SetToolTip("Move selected macro up");
    m_moveDownBtn->SetToolTip("Move selected macro down");
    m_resetBtn->SetToolTip("Reset to default macro commands");
    m_importBtn->SetToolTip("Import macros from file");
    m_exportBtn->SetToolTip("Export macros to file");
}

void MacroConfigDialog::UpdateMacroList()
{
    m_macroList->DeleteAllItems();
    
    for (size_t i = 0; i < m_macros.size(); ++i) {
        const auto& macro = m_macros[i];
        
        long itemIndex = m_macroList->InsertItem(i, macro.label);
        m_macroList->SetItem(itemIndex, 1, macro.command);
        m_macroList->SetItem(itemIndex, 2, macro.description);
    }
}

void MacroConfigDialog::UpdateButtonStates()
{
    int selected = GetSelectedIndex();
    bool hasSelection = (selected >= 0);
    bool hasItems = !m_macros.empty();
    
    m_editBtn->Enable(hasSelection);
    m_deleteBtn->Enable(hasSelection);
    m_moveUpBtn->Enable(hasSelection && selected > 0);
    m_moveDownBtn->Enable(hasSelection && selected < (int)m_macros.size() - 1);
    m_exportBtn->Enable(hasItems);
}

int MacroConfigDialog::GetSelectedIndex() const
{
    long selected = m_macroList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    return selected;
}

void MacroConfigDialog::SelectItem(int index)
{
    if (index >= 0 && index < (int)m_macros.size()) {
        m_macroList->SetItemState(index, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        m_macroList->EnsureVisible(index);
    }
}

std::vector<MacroDefinition> MacroConfigDialog::GetDefaultMacros() const
{
    return {
        {"$", "$", "Single status report"},
        {"$$", "$$", "Double status report (detailed)"},
        {"Reset", "\\x18", "Soft reset (Ctrl-X)"},
        {"Home", "$H", "Homing cycle"},
        {"Unlock", "$X", "Kill alarm lock"}
    };
}

void MacroConfigDialog::OnAdd(wxCommandEvent& WXUNUSED(event))
{
    MacroDefinition newMacro;
    MacroEditDialog dialog(this, newMacro, "Add New Macro");
    
    if (dialog.ShowModal() == wxID_OK) {
        AddMacro(dialog.GetMacro());
    }
}

void MacroConfigDialog::OnEdit(wxCommandEvent& WXUNUSED(event))
{
    int selected = GetSelectedIndex();
    if (selected >= 0) {
        EditMacro(selected);
    }
}

void MacroConfigDialog::OnDelete(wxCommandEvent& WXUNUSED(event))
{
    int selected = GetSelectedIndex();
    if (selected >= 0) {
        DeleteMacro(selected);
    }
}

void MacroConfigDialog::OnMoveUp(wxCommandEvent& WXUNUSED(event))
{
    int selected = GetSelectedIndex();
    if (selected > 0) {
        MoveMacroUp(selected);
    }
}

void MacroConfigDialog::OnMoveDown(wxCommandEvent& WXUNUSED(event))
{
    int selected = GetSelectedIndex();
    if (selected >= 0 && selected < (int)m_macros.size() - 1) {
        MoveMacroDown(selected);
    }
}

void MacroConfigDialog::OnReset(wxCommandEvent& WXUNUSED(event))
{
    if (wxMessageBox("Reset all macros to defaults?\nThis will remove any custom macros you have created.",
                     "Reset Macros", wxYES_NO | wxICON_QUESTION, this) == wxYES) {
        ResetToDefaults();
    }
}

void MacroConfigDialog::OnImport(wxCommandEvent& WXUNUSED(event))
{
    ImportMacros();
}

void MacroConfigDialog::OnExport(wxCommandEvent& WXUNUSED(event))
{
    ExportMacros();
}

void MacroConfigDialog::OnOK(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_OK);
}

void MacroConfigDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_CANCEL);
}

void MacroConfigDialog::OnSelectionChanged(wxListEvent& WXUNUSED(event))
{
    UpdateButtonStates();
}

void MacroConfigDialog::OnItemActivated(wxListEvent& event)
{
    EditMacro(event.GetIndex());
}

void MacroConfigDialog::AddMacro(const MacroDefinition& macro)
{
    m_macros.push_back(macro);
    m_modified = true;
    UpdateMacroList();
    SelectItem(m_macros.size() - 1);
    UpdateButtonStates();
}

void MacroConfigDialog::EditMacro(int index)
{
    if (index >= 0 && index < (int)m_macros.size()) {
        MacroEditDialog dialog(this, m_macros[index], "Edit Macro");
        
        if (dialog.ShowModal() == wxID_OK) {
            m_macros[index] = dialog.GetMacro();
            m_modified = true;
            UpdateMacroList();
            SelectItem(index);
        }
    }
}

void MacroConfigDialog::DeleteMacro(int index)
{
    if (index >= 0 && index < (int)m_macros.size()) {
        const auto& macro = m_macros[index];
        
        if (wxMessageBox(wxString::Format("Delete macro '%s'?", macro.label),
                         "Delete Macro", wxYES_NO | wxICON_QUESTION, this) == wxYES) {
            m_macros.erase(m_macros.begin() + index);
            m_modified = true;
            UpdateMacroList();
            
            // Select next item or previous if at end
            if (index < (int)m_macros.size()) {
                SelectItem(index);
            } else if (!m_macros.empty()) {
                SelectItem(m_macros.size() - 1);
            }
            UpdateButtonStates();
        }
    }
}

void MacroConfigDialog::MoveMacroUp(int index)
{
    if (index > 0 && index < (int)m_macros.size()) {
        std::swap(m_macros[index], m_macros[index - 1]);
        m_modified = true;
        UpdateMacroList();
        SelectItem(index - 1);
        UpdateButtonStates();
    }
}

void MacroConfigDialog::MoveMacroDown(int index)
{
    if (index >= 0 && index < (int)m_macros.size() - 1) {
        std::swap(m_macros[index], m_macros[index + 1]);
        m_modified = true;
        UpdateMacroList();
        SelectItem(index + 1);
        UpdateButtonStates();
    }
}

void MacroConfigDialog::ResetToDefaults()
{
    m_macros = GetDefaultMacros();
    m_modified = true;
    UpdateMacroList();
    UpdateButtonStates();
    
    NotificationSystem::Instance().ShowSuccess("Macros Reset", "Macros have been reset to defaults");
}

bool MacroConfigDialog::ImportMacros()
{
    wxFileDialog dialog(this, "Import Macros", "", "",
                       "Macro files (*.txt)|*.txt|All files (*.*)|*.*",
                       wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (dialog.ShowModal() != wxID_OK) return false;
    
    wxTextFile file;
    if (!file.Open(dialog.GetPath())) {
        NotificationSystem::Instance().ShowError("Import Failed", 
            "Could not open file: " + dialog.GetPath());
        return false;
    }
    
    std::vector<MacroDefinition> imported;
    for (size_t i = 0; i < file.GetLineCount(); ++i) {
        wxString line = file.GetLine(i).Trim();
        if (line.IsEmpty() || line.StartsWith("#")) continue;
        
        // Parse format: label|command|description
        wxArrayString parts = wxSplit(line, '|');
        if (parts.size() >= 2) {
            MacroDefinition macro;
            macro.label = parts[0].Trim().ToStdString();
            macro.command = parts[1].Trim().ToStdString();
            macro.description = (parts.size() > 2) ? parts[2].Trim().ToStdString() : "";
            imported.push_back(macro);
        }
    }
    
    file.Close();
    
    if (imported.empty()) {
        NotificationSystem::Instance().ShowWarning("Import Warning", "No valid macros found in file");
        return false;
    }
    
    if (wxMessageBox(wxString::Format("Import %zu macros?\nThis will replace all current macros.", imported.size()),
                     "Import Macros", wxYES_NO | wxICON_QUESTION, this) == wxYES) {
        m_macros = imported;
        m_modified = true;
        UpdateMacroList();
        UpdateButtonStates();
        
        NotificationSystem::Instance().ShowSuccess("Import Successful", 
            wxString::Format("Imported %zu macros", imported.size()));
        return true;
    }
    
    return false;
}

bool MacroConfigDialog::ExportMacros()
{
    if (m_macros.empty()) {
        NotificationSystem::Instance().ShowWarning("Export Warning", "No macros to export");
        return false;
    }
    
    wxFileDialog dialog(this, "Export Macros", "", "macros.txt",
                       "Macro files (*.txt)|*.txt|All files (*.*)|*.*",
                       wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dialog.ShowModal() != wxID_OK) return false;
    
    wxTextFile file;
    if (!file.Create(dialog.GetPath()) && !file.Open(dialog.GetPath())) {
        NotificationSystem::Instance().ShowError("Export Failed", 
            "Could not create file: " + dialog.GetPath());
        return false;
    }
    
    file.Clear();
    file.AddLine("# FluidNC Quick Command Macros");
    file.AddLine("# Format: label|command|description");
    file.AddLine("");
    
    for (const auto& macro : m_macros) {
        wxString line = wxString::Format("%s|%s|%s", 
            macro.label, macro.command, macro.description);
        file.AddLine(line);
    }
    
    bool success = file.Write();
    file.Close();
    
    if (success) {
        NotificationSystem::Instance().ShowSuccess("Export Successful", 
            wxString::Format("Exported %zu macros to %s", m_macros.size(), dialog.GetFilename()));
        return true;
    } else {
        NotificationSystem::Instance().ShowError("Export Failed", "Could not write to file");
        return false;
    }
}

// MacroEditDialog implementation

wxBEGIN_EVENT_TABLE(MacroEditDialog, wxDialog)
    EVT_BUTTON(wxID_OK, MacroEditDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, MacroEditDialog::OnCancel)
    EVT_TEXT(ID_LABEL_TEXT, MacroEditDialog::OnLabelChanged)
    EVT_TEXT(ID_COMMAND_TEXT, MacroEditDialog::OnCommandChanged)
wxEND_EVENT_TABLE()

MacroEditDialog::MacroEditDialog(wxWindow* parent, const MacroDefinition& macro, const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(450, 300))
    , m_macro(macro)
{
    CreateControls();
    
    // Set initial values
    m_labelText->SetValue(macro.label);
    m_commandText->SetValue(macro.command);
    m_descriptionText->SetValue(macro.description);
    
    UpdatePreview();
    
    // Focus on label field
    m_labelText->SetFocus();
    
    CenterOnParent();
}

MacroEditDialog::~MacroEditDialog()
{
}

void MacroEditDialog::CreateControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Form fields
    wxFlexGridSizer* formSizer = new wxFlexGridSizer(3, 2, 8, 10);
    formSizer->AddGrowableCol(1, 1);
    
    // Label
    formSizer->Add(new wxStaticText(this, wxID_ANY, "Label:"), 0, wxALIGN_CENTER_VERTICAL);
    m_labelText = new wxTextCtrl(this, ID_LABEL_TEXT, "", wxDefaultPosition, wxSize(200, -1));
    m_labelText->SetMaxLength(20);
    formSizer->Add(m_labelText, 0, wxEXPAND);
    
    // Command
    formSizer->Add(new wxStaticText(this, wxID_ANY, "Command:"), 0, wxALIGN_CENTER_VERTICAL);
    m_commandText = new wxTextCtrl(this, ID_COMMAND_TEXT, "", wxDefaultPosition, wxSize(200, -1));
    m_commandText->SetMaxLength(100);
    formSizer->Add(m_commandText, 0, wxEXPAND);
    
    // Description
    formSizer->Add(new wxStaticText(this, wxID_ANY, "Description:"), 0, wxALIGN_CENTER_VERTICAL);
    m_descriptionText = new wxTextCtrl(this, ID_DESCRIPTION_TEXT, "", wxDefaultPosition, wxSize(200, -1));
    m_descriptionText->SetMaxLength(200);
    formSizer->Add(m_descriptionText, 0, wxEXPAND);
    
    mainSizer->Add(formSizer, 0, wxEXPAND | wxALL, 15);
    
    // Preview
    wxStaticBoxSizer* previewSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Preview");
    m_previewText = new wxStaticText(this, wxID_ANY, "");
    m_previewText->SetFont(m_previewText->GetFont().Scale(0.9));
    previewSizer->Add(m_previewText, 0, wxEXPAND | wxALL, 8);
    mainSizer->Add(previewSizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 15);
    
    // Buttons
    mainSizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 15);
    
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_okBtn = new wxButton(this, wxID_OK, "OK");
    m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(m_okBtn, 0, wxRIGHT, 5);
    buttonSizer->Add(m_cancelBtn, 0);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 15);
    
    SetSizer(mainSizer);
    
    m_okBtn->SetDefault();
}

void MacroEditDialog::UpdatePreview()
{
    wxString label = m_labelText->GetValue().Trim();
    wxString command = m_commandText->GetValue().Trim();
    wxString description = m_descriptionText->GetValue().Trim();
    
    wxString preview;
    if (!label.IsEmpty()) {
        preview += wxString::Format("Button label: \"%s\"\n", label);
    }
    if (!command.IsEmpty()) {
        preview += wxString::Format("Command to send: \"%s\"\n", command);
    }
    if (!description.IsEmpty()) {
        preview += wxString::Format("Tooltip: \"%s\"", description);
    }
    
    if (preview.IsEmpty()) {
        preview = "Enter label and command to see preview";
    }
    
    m_previewText->SetLabel(preview);
}

bool MacroEditDialog::ValidateInput()
{
    wxString label = m_labelText->GetValue().Trim();
    wxString command = m_commandText->GetValue().Trim();
    
    if (label.IsEmpty()) {
        NotificationSystem::Instance().ShowWarning("Validation Error", "Label cannot be empty");
        m_labelText->SetFocus();
        return false;
    }
    
    if (command.IsEmpty()) {
        NotificationSystem::Instance().ShowWarning("Validation Error", "Command cannot be empty");
        m_commandText->SetFocus();
        return false;
    }
    
    return true;
}

MacroDefinition MacroEditDialog::GetMacro() const
{
    MacroDefinition macro;
    macro.label = m_labelText->GetValue().Trim().ToStdString();
    macro.command = m_commandText->GetValue().Trim().ToStdString();
    macro.description = m_descriptionText->GetValue().Trim().ToStdString();
    return macro;
}

void MacroEditDialog::OnOK(wxCommandEvent& WXUNUSED(event))
{
    if (ValidateInput()) {
        EndModal(wxID_OK);
    }
}

void MacroEditDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_CANCEL);
}

void MacroEditDialog::OnLabelChanged(wxCommandEvent& WXUNUSED(event))
{
    UpdatePreview();
}

void MacroEditDialog::OnCommandChanged(wxCommandEvent& WXUNUSED(event))
{
    UpdatePreview();
}
