/**
 * gui/ProjectInfoDialog.cpp
 * Dialog for editing project implementation status and ToDo list
 */

#include "ProjectInfoDialog.h"
#include "core/SimpleLogger.h"
#include "NotificationSystem.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/datetime.h>
#include <fstream>
#include "../../external/json.hpp"
#include <wx/config.h>

// Event table
wxBEGIN_EVENT_TABLE(ProjectInfoDialog, wxDialog)
    EVT_BUTTON(wxID_OK, ProjectInfoDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, ProjectInfoDialog::OnCancel)
    EVT_BUTTON(ID_SAVE, ProjectInfoDialog::OnSave)
    EVT_BUTTON(ID_LOAD, ProjectInfoDialog::OnLoad)
    EVT_BUTTON(ID_ADD_IMPLEMENTED, ProjectInfoDialog::OnAddImplemented)
    EVT_BUTTON(ID_ADD_TODO, ProjectInfoDialog::OnAddToDo)
    EVT_BUTTON(ID_EDIT_ITEM, ProjectInfoDialog::OnEditItem)
    EVT_BUTTON(ID_DELETE_ITEM, ProjectInfoDialog::OnDeleteItem)
    EVT_LIST_ITEM_SELECTED(wxID_ANY, ProjectInfoDialog::OnListItemSelected)
    EVT_LIST_ITEM_ACTIVATED(wxID_ANY, ProjectInfoDialog::OnListItemActivated)
wxEND_EVENT_TABLE()

ProjectInfoDialog::ProjectInfoDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Project Information - Implementation Status & ToDo List", 
               wxDefaultPosition, wxSize(900, 700), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_splitter(nullptr)
    , m_topPanel(nullptr)
    , m_bottomPanel(nullptr)
    , m_implementedList(nullptr)
    , m_addImplementedBtn(nullptr)
    , m_editImplementedBtn(nullptr)
    , m_deleteImplementedBtn(nullptr)
    , m_todoList(nullptr)
    , m_addToDoBtn(nullptr)
    , m_editToDoBtn(nullptr)
    , m_deleteToDoBtn(nullptr)
    , m_saveBtn(nullptr)
    , m_loadBtn(nullptr)
    , m_okBtn(nullptr)
    , m_cancelBtn(nullptr)
{
    CreateControls();
    LoadProjectInfo();
    LoadWindowSettings();
}

ProjectInfoDialog::~ProjectInfoDialog()
{
    // Save data and window settings before closing
    SaveWindowSettings();
    SaveProjectInfo();
}

void ProjectInfoDialog::CreateControls()
{
    // Main vertical sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    wxStaticText* title = new wxStaticText(this, wxID_ANY, "FluidNC gCode Sender - Project Status Tracking");
    wxFont titleFont = title->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 2);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    mainSizer->Add(title, 0, wxALL | wxALIGN_CENTER, 10);
    
    // Create splitter window for implemented vs todo lists
    m_splitter = new wxSplitterWindow(this, wxID_ANY);
    
    // Top panel for implemented features
    m_topPanel = new wxPanel(m_splitter, wxID_ANY);
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    
    // Implemented features section
    wxStaticText* implementedLabel = new wxStaticText(m_topPanel, wxID_ANY, "[DONE] Implemented Features");
    wxFont labelFont = implementedLabel->GetFont();
    labelFont.SetWeight(wxFONTWEIGHT_BOLD);
    implementedLabel->SetFont(labelFont);
    topSizer->Add(implementedLabel, 0, wxALL, 5);
    
    // Implemented features list
    m_implementedList = new wxListCtrl(m_topPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
                                       wxLC_REPORT | wxLC_SINGLE_SEL);
    m_implementedList->AppendColumn("Feature", wxLIST_FORMAT_LEFT, 250);
    m_implementedList->AppendColumn("Description", wxLIST_FORMAT_LEFT, 450);
    m_implementedList->AppendColumn("Date Added", wxLIST_FORMAT_LEFT, 90);
    topSizer->Add(m_implementedList, 1, wxEXPAND | wxALL, 5);
    
    // Implemented features buttons
    wxBoxSizer* implementedBtnSizer = new wxBoxSizer(wxHORIZONTAL);
    m_addImplementedBtn = new wxButton(m_topPanel, ID_ADD_IMPLEMENTED, "Add Feature");
    m_editImplementedBtn = new wxButton(m_topPanel, ID_EDIT_ITEM, "Edit");
    m_deleteImplementedBtn = new wxButton(m_topPanel, ID_DELETE_ITEM, "Delete");
    implementedBtnSizer->Add(m_addImplementedBtn, 0, wxRIGHT, 5);
    implementedBtnSizer->Add(m_editImplementedBtn, 0, wxRIGHT, 5);
    implementedBtnSizer->Add(m_deleteImplementedBtn, 0, 0, 0);
    implementedBtnSizer->AddStretchSpacer();
    topSizer->Add(implementedBtnSizer, 0, wxEXPAND | wxALL, 5);
    
    m_topPanel->SetSizer(topSizer);
    
    // Bottom panel for todo items
    m_bottomPanel = new wxPanel(m_splitter, wxID_ANY);
    wxBoxSizer* bottomSizer = new wxBoxSizer(wxVERTICAL);
    
    // ToDo section
    wxStaticText* todoLabel = new wxStaticText(m_bottomPanel, wxID_ANY, "[TODO] Task List");
    todoLabel->SetFont(labelFont);
    bottomSizer->Add(todoLabel, 0, wxALL, 5);
    
    // ToDo list
    m_todoList = new wxListCtrl(m_bottomPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
                               wxLC_REPORT | wxLC_SINGLE_SEL);
    m_todoList->AppendColumn("Task", wxLIST_FORMAT_LEFT, 200);
    m_todoList->AppendColumn("Priority", wxLIST_FORMAT_LEFT, 70);
    m_todoList->AppendColumn("Description", wxLIST_FORMAT_LEFT, 440);
    m_todoList->AppendColumn("Date Added", wxLIST_FORMAT_LEFT, 90);
    bottomSizer->Add(m_todoList, 1, wxEXPAND | wxALL, 5);
    
    // ToDo buttons
    wxBoxSizer* todoBtnSizer = new wxBoxSizer(wxHORIZONTAL);
    m_addToDoBtn = new wxButton(m_bottomPanel, ID_ADD_TODO, "Add ToDo");
    m_editToDoBtn = new wxButton(m_bottomPanel, ID_EDIT_ITEM, "Edit");
    m_deleteToDoBtn = new wxButton(m_bottomPanel, ID_DELETE_ITEM, "Delete");
    todoBtnSizer->Add(m_addToDoBtn, 0, wxRIGHT, 5);
    todoBtnSizer->Add(m_editToDoBtn, 0, wxRIGHT, 5);
    todoBtnSizer->Add(m_deleteToDoBtn, 0, 0, 0);
    todoBtnSizer->AddStretchSpacer();
    bottomSizer->Add(todoBtnSizer, 0, wxEXPAND | wxALL, 5);
    
    m_bottomPanel->SetSizer(bottomSizer);
    
    // Split the window
    m_splitter->SplitHorizontally(m_topPanel, m_bottomPanel);
    m_splitter->SetMinimumPaneSize(150);
    m_splitter->SetSashGravity(0.5); // Split 50/50
    
    mainSizer->Add(m_splitter, 1, wxEXPAND | wxALL, 10);
    
    // Bottom buttons
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    m_saveBtn = new wxButton(this, ID_SAVE, "Save to File");
    m_loadBtn = new wxButton(this, ID_LOAD, "Load from File");
    btnSizer->Add(m_saveBtn, 0, wxRIGHT, 5);
    btnSizer->Add(m_loadBtn, 0, wxRIGHT, 10);
    btnSizer->AddStretchSpacer();
    
    m_okBtn = new wxButton(this, wxID_OK, "OK");
    m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    btnSizer->Add(m_okBtn, 0, wxRIGHT, 5);
    btnSizer->Add(m_cancelBtn, 0, 0, 0);
    
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(mainSizer);
    
    // Set default button
    m_okBtn->SetDefault();
    
    // Enable/disable edit/delete buttons initially
    m_editImplementedBtn->Enable(false);
    m_deleteImplementedBtn->Enable(false);
    m_editToDoBtn->Enable(false);
    m_deleteToDoBtn->Enable(false);
}

void ProjectInfoDialog::LoadProjectInfo()
{
    // Add some default implemented features if none exist
    if (m_implementedItems.empty()) {
        AddImplementedItem("Basic Application Framework", "Main window with AUI docking system");
        AddImplementedItem("Machine Manager Panel", "Add, edit, and manage CNC machine configurations");
        AddImplementedItem("Console Terminal", "Command line interface with history and session logging");
        AddImplementedItem("Connection Management", "Serial port and network connection handling");
        AddImplementedItem("Build System", "MinGW-based build with CMAKE and automated build scripts");
        AddImplementedItem("Error Handling System", "Centralized error reporting with user-friendly dialogs");
        AddImplementedItem("Notification System", "Non-blocking toast notifications for user feedback");
        AddImplementedItem("State Management", "Persistent application settings and window layouts");
        AddImplementedItem("Simple Logger", "File-based logging system with multiple log levels");
        AddImplementedItem("DRO (Digital Readout)", "Real-time position display for machine coordinates");
        AddImplementedItem("Jogging Controls", "Manual machine movement controls with configurable steps");
        AddImplementedItem("G-code Editor", "Basic text editor for G-code files with syntax highlighting");
        AddImplementedItem("SVG Viewer", "Basic SVG file viewing capability");
        AddImplementedItem("Macro System", "Configurable macro buttons for common commands");
    }
    
    // Add some default todo items if none exist
    if (m_todoItems.empty()) {
        AddToDoItem("G-code Sender", "High", "Implement G-code streaming to CNC machine");
        AddToDoItem("Real-time Status", "High", "Parse and display FluidNC status reports");
        AddToDoItem("Job Progress Tracking", "Medium", "Show progress bar and estimated completion time");
        AddToDoItem("Tool Path Visualization", "Medium", "2D/3D preview of G-code tool paths");
        AddToDoItem("Alarm Handling", "High", "Proper handling of machine alarms and error states");
        AddToDoItem("Probing Functions", "Medium", "Touch probe and work coordinate system setup");
        AddToDoItem("Advanced Macros", "Low", "Conditional macros and variable substitution");
        AddToDoItem("Plugin System", "Low", "Extensible plugin architecture for custom features");
        AddToDoItem("Network Discovery", "Medium", "Automatic discovery of FluidNC devices on network");
        AddToDoItem("File Management", "Medium", "Local and remote file browsing and management");
        AddToDoItem("Settings Import/Export", "Low", "Backup and restore application configurations");
        AddToDoItem("Multi-language Support", "Low", "Internationalization and localization support");
    }
    
    // Try to load from file if it exists
    LoadFromFile();
    
    // Refresh the UI lists
    RefreshLists();
}

void ProjectInfoDialog::SaveProjectInfo()
{
    SaveToFile();
}

void ProjectInfoDialog::RefreshLists()
{
    PopulateImplementedList();
    PopulateToDoList();
}

void ProjectInfoDialog::PopulateImplementedList()
{
    m_implementedList->DeleteAllItems();
    
    for (size_t i = 0; i < m_implementedItems.size(); ++i) {
        const auto& item = m_implementedItems[i];
        long index = m_implementedList->InsertItem(i, item.feature);
        m_implementedList->SetItem(index, 1, item.description);
        m_implementedList->SetItem(index, 2, item.dateAdded);
        m_implementedList->SetItemData(index, i);
    }
}

void ProjectInfoDialog::PopulateToDoList()
{
    m_todoList->DeleteAllItems();
    
    for (size_t i = 0; i < m_todoItems.size(); ++i) {
        const auto& item = m_todoItems[i];
        long index = m_todoList->InsertItem(i, item.task);
        m_todoList->SetItem(index, 1, item.priority);
        m_todoList->SetItem(index, 2, item.description);
        m_todoList->SetItem(index, 3, item.dateAdded);
        
        // Color coding for priorities
        if (item.priority == "High") {
            m_todoList->SetItemTextColour(index, *wxRED);
        } else if (item.priority == "Medium") {
            m_todoList->SetItemTextColour(index, wxColour(255, 140, 0)); // Orange
        } else {
            m_todoList->SetItemTextColour(index, *wxBLUE);
        }
        
        m_todoList->SetItemData(index, i);
    }
}

void ProjectInfoDialog::AddImplementedItem(const wxString& feature, const wxString& description)
{
    ImplementedItem item;
    item.feature = feature;
    item.description = description;
    item.dateAdded = wxDateTime::Now().FormatISODate();
    m_implementedItems.push_back(item);
}

void ProjectInfoDialog::AddToDoItem(const wxString& task, const wxString& priority, const wxString& description)
{
    ToDoItem item;
    item.task = task;
    item.priority = priority;
    item.description = description;
    item.dateAdded = wxDateTime::Now().FormatISODate();
    m_todoItems.push_back(item);
}

wxString ProjectInfoDialog::GetProjectInfoFilePath() const
{
    wxString appDir = wxGetCwd();
    // Check if we're in a build directory, if so go to parent
    if (appDir.Contains("build")) {
        // Navigate to project root from build directory
        wxFileName projectRoot(appDir);
        projectRoot.RemoveLastDir(); // Remove build-* directory
        appDir = projectRoot.GetPath();
    }
    return appDir + wxFILE_SEP_PATH + "docs" + wxFILE_SEP_PATH + "ProjectInfo.json";
}

void ProjectInfoDialog::LoadFromFile()
{
    wxString filePath = GetProjectInfoFilePath();
    
    if (!wxFileExists(filePath)) {
        LOG_INFO("Project info file does not exist, using defaults: " + filePath.ToStdString());
        return;
    }
    
    try {
        std::ifstream file(filePath.ToStdString());
        if (!file.is_open()) {
            LOG_ERROR("Failed to open project info file: " + filePath.ToStdString());
            return;
        }
        
        nlohmann::json root;
        file >> root;
        file.close();
        
        // Clear existing data
        m_implementedItems.clear();
        m_todoItems.clear();
        
        // Load implemented features
        if (root.contains("implemented") && root["implemented"].is_array()) {
            for (const auto& item : root["implemented"]) {
                ImplementedItem implItem;
                implItem.feature = wxString::FromUTF8(item.value("feature", "").c_str());
                implItem.description = wxString::FromUTF8(item.value("description", "").c_str());
                implItem.dateAdded = wxString::FromUTF8(item.value("dateAdded", "").c_str());
                m_implementedItems.push_back(implItem);
            }
        }
        
        // Load todo items
        if (root.contains("todo") && root["todo"].is_array()) {
            for (const auto& item : root["todo"]) {
                ToDoItem todoItem;
                todoItem.task = wxString::FromUTF8(item.value("task", "").c_str());
                todoItem.priority = wxString::FromUTF8(item.value("priority", "Medium").c_str());
                todoItem.description = wxString::FromUTF8(item.value("description", "").c_str());
                todoItem.dateAdded = wxString::FromUTF8(item.value("dateAdded", "").c_str());
                m_todoItems.push_back(todoItem);
            }
        }
        
        LOG_INFO("Loaded project info from file: " + filePath.ToStdString());
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception loading project info file: " + std::string(e.what()));
    }
}

void ProjectInfoDialog::SaveToFile()
{
    wxString filePath = GetProjectInfoFilePath();
    
    try {
        nlohmann::json root;
        
        // Save implemented features
        nlohmann::json implemented = nlohmann::json::array();
        for (const auto& item : m_implementedItems) {
            nlohmann::json implItem;
            implItem["feature"] = item.feature.ToUTF8().data();
            implItem["description"] = item.description.ToUTF8().data();
            implItem["dateAdded"] = item.dateAdded.ToUTF8().data();
            implemented.push_back(implItem);
        }
        root["implemented"] = implemented;
        
        // Save todo items
        nlohmann::json todo = nlohmann::json::array();
        for (const auto& item : m_todoItems) {
            nlohmann::json todoItem;
            todoItem["task"] = item.task.ToUTF8().data();
            todoItem["priority"] = item.priority.ToUTF8().data();
            todoItem["description"] = item.description.ToUTF8().data();
            todoItem["dateAdded"] = item.dateAdded.ToUTF8().data();
            todo.push_back(todoItem);
        }
        root["todo"] = todo;
        
        // Write to file
        std::ofstream file(filePath.ToStdString());
        if (!file.is_open()) {
            LOG_ERROR("Failed to create project info file: " + filePath.ToStdString());
            return;
        }
        
        file << root.dump(4); // Pretty print with 4-space indentation
        file.close();
        
        LOG_INFO("Saved project info to file: " + filePath.ToStdString());
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception saving project info file: " + std::string(e.what()));
    }
}

void ProjectInfoDialog::LoadWindowSettings()
{
    wxConfigBase* config = wxConfigBase::Get();
    if (!config) return;
    
    // Load window position and size
    int x = config->Read("ProjectInfoDialog/PosX", -1);
    int y = config->Read("ProjectInfoDialog/PosY", -1);
    int w = config->Read("ProjectInfoDialog/Width", 900);
    int h = config->Read("ProjectInfoDialog/Height", 700);
    
    // Validate position is on screen
    if (x >= 0 && y >= 0) {
        SetPosition(wxPoint(x, y));
    } else {
        Center();
    }
    SetSize(wxSize(w, h));
    
    // Load splitter position
    int sashPos = config->Read("ProjectInfoDialog/SashPos", -1);
    if (sashPos > 0 && m_splitter) {
        // Set it after the window is shown to ensure proper sizing
        CallAfter([this, sashPos]() {
            m_splitter->SetSashPosition(sashPos);
        });
    }
}

void ProjectInfoDialog::SaveWindowSettings()
{
    wxConfigBase* config = wxConfigBase::Get();
    if (!config) return;
    
    // Save window position and size
    wxPoint pos = GetPosition();
    wxSize size = GetSize();
    
    config->Write("ProjectInfoDialog/PosX", pos.x);
    config->Write("ProjectInfoDialog/PosY", pos.y);
    config->Write("ProjectInfoDialog/Width", size.x);
    config->Write("ProjectInfoDialog/Height", size.y);
    
    // Save splitter position
    if (m_splitter) {
        config->Write("ProjectInfoDialog/SashPos", m_splitter->GetSashPosition());
    }
    
    config->Flush();
}

// Event handlers
void ProjectInfoDialog::OnOK(wxCommandEvent& WXUNUSED(event))
{
    SaveProjectInfo();
    EndModal(wxID_OK);
}

void ProjectInfoDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_CANCEL);
}

void ProjectInfoDialog::OnSave(wxCommandEvent& WXUNUSED(event))
{
    SaveToFile();
    NOTIFY_SUCCESS("Project Info Saved", "Project information has been saved to ProjectInfo.json");
}

void ProjectInfoDialog::OnLoad(wxCommandEvent& WXUNUSED(event))
{
    LoadFromFile();
    RefreshLists();
    NOTIFY_INFO("Project Info Loaded", "Project information has been reloaded from ProjectInfo.json");
}

void ProjectInfoDialog::OnAddImplemented(wxCommandEvent& WXUNUSED(event))
{
    wxTextEntryDialog featureDialog(this, "Enter implemented feature name:", "Add Implemented Feature");
    
    if (featureDialog.ShowModal() == wxID_OK) {
        wxString feature = featureDialog.GetValue().Trim();
        if (!feature.IsEmpty()) {
            wxTextEntryDialog descDialog(this, "Enter feature description (optional):", "Feature Description");
            wxString description;
            if (descDialog.ShowModal() == wxID_OK) {
                description = descDialog.GetValue().Trim();
            }
            
            AddImplementedItem(feature, description);
            RefreshLists();
        }
    }
}

void ProjectInfoDialog::OnAddToDo(wxCommandEvent& WXUNUSED(event))
{
    wxTextEntryDialog taskDialog(this, "Enter todo task name:", "Add ToDo Item");
    
    if (taskDialog.ShowModal() == wxID_OK) {
        wxString task = taskDialog.GetValue().Trim();
        if (!task.IsEmpty()) {
            // Priority selection
            wxString priorities[] = {"High", "Medium", "Low"};
            wxSingleChoiceDialog priorityDialog(this, "Select priority:", "Task Priority", 3, priorities);
            priorityDialog.SetSelection(1); // Default to Medium
            
            wxString priority = "Medium";
            if (priorityDialog.ShowModal() == wxID_OK) {
                priority = priorityDialog.GetStringSelection();
            }
            
            // Description
            wxTextEntryDialog descDialog(this, "Enter task description (optional):", "Task Description");
            wxString description;
            if (descDialog.ShowModal() == wxID_OK) {
                description = descDialog.GetValue().Trim();
            }
            
            AddToDoItem(task, priority, description);
            RefreshLists();
        }
    }
}

void ProjectInfoDialog::OnEditItem(wxCommandEvent& WXUNUSED(event))
{
    // Determine which list is focused
    wxWindow* focused = FindFocus();
    
    if (focused == m_implementedList) {
        long selected = m_implementedList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (selected >= 0 && selected < (long)m_implementedItems.size()) {
            auto& item = m_implementedItems[selected];
            
            wxTextEntryDialog featureDialog(this, "Edit feature name:", "Edit Implemented Feature", item.feature);
            if (featureDialog.ShowModal() == wxID_OK) {
                wxString feature = featureDialog.GetValue().Trim();
                if (!feature.IsEmpty()) {
                    item.feature = feature;
                    
                    wxTextEntryDialog descDialog(this, "Edit feature description:", "Feature Description", item.description);
                    if (descDialog.ShowModal() == wxID_OK) {
                        item.description = descDialog.GetValue().Trim();
                    }
                    
                    RefreshLists();
                }
            }
        }
    } else if (focused == m_todoList) {
        long selected = m_todoList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (selected >= 0 && selected < (long)m_todoItems.size()) {
            auto& item = m_todoItems[selected];
            
            wxTextEntryDialog taskDialog(this, "Edit task name:", "Edit ToDo Item", item.task);
            if (taskDialog.ShowModal() == wxID_OK) {
                wxString task = taskDialog.GetValue().Trim();
                if (!task.IsEmpty()) {
                    item.task = task;
                    
                    // Priority selection
                    wxString priorities[] = {"High", "Medium", "Low"};
                    wxSingleChoiceDialog priorityDialog(this, "Select priority:", "Task Priority", 3, priorities);
                    for (int i = 0; i < 3; ++i) {
                        if (priorities[i] == item.priority) {
                            priorityDialog.SetSelection(i);
                            break;
                        }
                    }
                    
                    if (priorityDialog.ShowModal() == wxID_OK) {
                        item.priority = priorityDialog.GetStringSelection();
                    }
                    
                    // Description
                    wxTextEntryDialog descDialog(this, "Edit task description:", "Task Description", item.description);
                    if (descDialog.ShowModal() == wxID_OK) {
                        item.description = descDialog.GetValue().Trim();
                    }
                    
                    RefreshLists();
                }
            }
        }
    }
}

void ProjectInfoDialog::OnDeleteItem(wxCommandEvent& WXUNUSED(event))
{
    wxWindow* focused = FindFocus();
    
    if (focused == m_implementedList) {
        long selected = m_implementedList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (selected >= 0 && selected < (long)m_implementedItems.size()) {
            wxString feature = m_implementedItems[selected].feature;
            if (wxMessageBox("Delete implemented feature '" + feature + "'?", "Confirm Delete", 
                           wxYES_NO | wxICON_QUESTION, this) == wxYES) {
                m_implementedItems.erase(m_implementedItems.begin() + selected);
                RefreshLists();
            }
        }
    } else if (focused == m_todoList) {
        long selected = m_todoList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (selected >= 0 && selected < (long)m_todoItems.size()) {
            wxString task = m_todoItems[selected].task;
            if (wxMessageBox("Delete todo item '" + task + "'?", "Confirm Delete", 
                           wxYES_NO | wxICON_QUESTION, this) == wxYES) {
                m_todoItems.erase(m_todoItems.begin() + selected);
                RefreshLists();
            }
        }
    }
}

void ProjectInfoDialog::OnListItemSelected(wxListEvent& event)
{
    wxObject* eventObject = event.GetEventObject();
    
    if (eventObject == m_implementedList) {
        bool hasSelection = m_implementedList->GetSelectedItemCount() > 0;
        m_editImplementedBtn->Enable(hasSelection);
        m_deleteImplementedBtn->Enable(hasSelection);
    } else if (eventObject == m_todoList) {
        bool hasSelection = m_todoList->GetSelectedItemCount() > 0;
        m_editToDoBtn->Enable(hasSelection);
        m_deleteToDoBtn->Enable(hasSelection);
    }
}

void ProjectInfoDialog::OnListItemActivated(wxListEvent& event)
{
    // Double-click to edit - convert to command event
    wxCommandEvent cmdEvent;
    OnEditItem(cmdEvent);
}
