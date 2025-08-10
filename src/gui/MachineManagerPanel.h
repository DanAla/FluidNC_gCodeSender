/**
 * gui/MachineManagerPanel.h
 * Machine configuration and connection management panel
 * Manages multiple machine profiles, connections, and configurations
 */

#pragma once

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <vector>
#include <string>
#include <map>

/**
 * Machine Manager Panel - manages machine configurations and connections
 * Features:
 * - Machine profile creation and editing
 * - Connection status monitoring
 * - Quick connect/disconnect operations
 * - Machine settings management
 */
class MachineManagerPanel : public wxPanel
{
public:
    MachineManagerPanel(wxWindow* parent);
    
    // Machine management
    void RefreshMachineList();
    void SelectMachine(const std::string& machineId);
    void UpdateConnectionStatus(const std::string& machineId, bool connected);

private:
    // Event handlers
    void OnMachineSelected(wxListEvent& event);
    void OnMachineActivated(wxListEvent& event);
    void OnAddMachine(wxCommandEvent& event);
    void OnEditMachine(wxCommandEvent& event);
    void OnRemoveMachine(wxCommandEvent& event);
    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnTestConnection(wxCommandEvent& event);
    void OnImportConfig(wxCommandEvent& event);
    void OnExportConfig(wxCommandEvent& event);
    
    // UI Creation
    void CreateControls();
    void CreateMachineList();
    void CreateMachineDetails();
    void CreateButtonPanel();
    
    // Data management
    void LoadMachineConfigs();
    void SaveMachineConfigs();
    void UpdateMachineDetails();
    void PopulateMachineList();
    
    // Settings management
    wxString GetSettingsPath();
    void CreateEmptyMachinesFile(const wxString& filePath);
    
    // Connection helper methods
    bool TestTelnetConnection(const std::string& host, int port);
    
    // UI Components
    wxSplitterWindow* m_splitter;
    
    // Machine list panel
    wxPanel* m_listPanel;
    wxListCtrl* m_machineList;
    wxButton* m_addBtn;
    wxButton* m_editBtn;
    wxButton* m_removeBtn;
    wxButton* m_importBtn;
    wxButton* m_exportBtn;
    
    // Machine details panel
    wxPanel* m_detailsPanel;
    wxStaticText* m_nameLabel;
    wxStaticText* m_descriptionLabel;
    wxStaticText* m_hostLabel;
    wxStaticText* m_portLabel;
    wxStaticText* m_statusLabel;
    wxStaticText* m_machineTypeLabel;
    wxStaticText* m_lastConnectedLabel;
    
    wxButton* m_connectBtn;
    wxButton* m_disconnectBtn;
    wxButton* m_testBtn;
    
    // Current selection
    std::string m_selectedMachine;
    
    // Machine configurations (dummy data for now)
    struct MachineConfig {
        std::string id;
        std::string name;
        std::string description;
        std::string host;
        int port;
        std::string machineType;
        bool connected;
        std::string lastConnected;
    };
    
    std::vector<MachineConfig> m_machines;
    
    wxDECLARE_EVENT_TABLE();
};
