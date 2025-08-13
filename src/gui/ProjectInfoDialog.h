/**
 * gui/ProjectInfoDialog.h
 * Dialog for editing project implementation status and ToDo list
 */

#pragma once

#include <wx/wx.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <string>

class ProjectInfoDialog : public wxDialog
{
public:
    ProjectInfoDialog(wxWindow* parent);
    ~ProjectInfoDialog();

private:
    // Event handlers
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnLoad(wxCommandEvent& event);
    void OnAddImplemented(wxCommandEvent& event);
    void OnAddToDo(wxCommandEvent& event);
    void OnEditItem(wxCommandEvent& event);
    void OnDeleteItem(wxCommandEvent& event);
    void OnListItemSelected(wxListEvent& event);
    void OnListItemActivated(wxListEvent& event);
    
    // UI creation
    void CreateControls();
    void LoadProjectInfo();
    void SaveProjectInfo();
    void PopulateImplementedList();
    void PopulateToDoList();
    void RefreshLists();
    
    // Data management
    void AddImplementedItem(const wxString& item, const wxString& description = "");
    void AddToDoItem(const wxString& item, const wxString& priority = "Medium", const wxString& description = "");
    
    // File operations
    wxString GetProjectInfoFilePath() const;
    void LoadFromFile();
    void SaveToFile();
    
    // Window settings persistence
    void LoadWindowSettings();
    void SaveWindowSettings();
    
    // UI elements
    wxSplitterWindow* m_splitter;
    wxPanel* m_topPanel;
    wxPanel* m_bottomPanel;
    
    // Implemented features list
    wxListCtrl* m_implementedList;
    wxButton* m_addImplementedBtn;
    wxButton* m_editImplementedBtn;
    wxButton* m_deleteImplementedBtn;
    
    // ToDo list
    wxListCtrl* m_todoList;
    wxButton* m_addToDoBtn;
    wxButton* m_editToDoBtn;
    wxButton* m_deleteToDoBtn;
    
    // Bottom buttons
    wxButton* m_saveBtn;
    wxButton* m_loadBtn;
    wxButton* m_okBtn;
    wxButton* m_cancelBtn;
    
    // Data storage
    struct ImplementedItem {
        wxString feature;
        wxString description;
        wxString dateAdded;
    };
    
    struct ToDoItem {
        wxString task;
        wxString priority;
        wxString description;
        wxString dateAdded;
    };
    
    std::vector<ImplementedItem> m_implementedItems;
    std::vector<ToDoItem> m_todoItems;
    
    wxDECLARE_EVENT_TABLE();
};

// Menu IDs
enum {
    ID_SAVE = wxID_HIGHEST + 1,
    ID_LOAD,
    ID_ADD_IMPLEMENTED,
    ID_ADD_TODO,
    ID_EDIT_ITEM,
    ID_DELETE_ITEM
};
