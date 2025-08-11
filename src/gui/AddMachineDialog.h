/**
 * gui/AddMachineDialog.h
 * Dialog for adding new machine configurations
 * Provides form interface for creating machine profiles
 */

#pragma once

#include <wx/wx.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>

/**
 * Add Machine Dialog - creates new machine configurations
 * Features:
 * - Machine name and description input
 * - Connection settings (host, port, protocol)
 * - Machine type selection
 * - Input validation
 * - Preview of configuration
 */
class AddMachineDialog : public wxDialog
{
public:
    AddMachineDialog(wxWindow* parent, bool isEditMode = false, const wxString& title = "Add New Machine");
    
    // Data structure for machine configuration
    struct MachineData {
        wxString name;
        wxString description;
        wxString host;
        int port;
        wxString protocol;
        wxString machineType;
        wxString baudRate;  // For USB/Serial connections
        wxString serialPort; // For USB/Serial connections
        bool autoConnect;   // Auto-connect on startup
    };
    
    // Get the configured machine data
    MachineData GetMachineData() const;
    
    // Set default values (for editing existing machines)
    void SetMachineData(const MachineData& data);

private:
    // Event handlers
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnProtocolChange(wxCommandEvent& event);
    void OnTestConnection(wxCommandEvent& event);
    void OnValidateInput(wxCommandEvent& event);
    
    // UI Creation
    void CreateControls();
    void CreateBasicSettings();
    void CreateConnectionSettings();
    void CreateAdvancedSettings();
    void CreateButtonPanel();
    
    // Validation and helpers
    bool ValidateInput();
    void UpdateConnectionFields();
    void EnableControls();
    
    // Connection testing
    bool TestTelnetConnection(const std::string& host, int port);
    
    // UI Components
    wxPanel* m_mainPanel;
    
    // Basic settings
    wxStaticBoxSizer* m_basicSizer;
    wxTextCtrl* m_nameCtrl;
    wxTextCtrl* m_descriptionCtrl;
    wxChoice* m_machineTypeCtrl;
    
    // Connection settings
    wxStaticBoxSizer* m_connectionSizer;
    wxChoice* m_protocolCtrl;
    wxTextCtrl* m_hostCtrl;
    wxSpinCtrl* m_portCtrl;
    wxChoice* m_serialPortCtrl;
    wxChoice* m_baudRateCtrl;
    
    // Connection type labels (dynamic visibility)
    wxStaticText* m_hostLabel;
    wxStaticText* m_portLabel;
    wxStaticText* m_serialPortLabel;
    wxStaticText* m_baudRateLabel;
    
    // Advanced settings
    wxStaticBoxSizer* m_advancedSizer;
    wxCheckBox* m_autoConnectCtrl;
    wxCheckBox* m_enableLoggingCtrl;
    wxSpinCtrl* m_timeoutCtrl;
    wxSpinCtrl* m_retryCountCtrl;
    
    // Buttons
    wxButton* m_okBtn;
    wxButton* m_cancelBtn;
    wxButton* m_testBtn;
    
    // State
    bool m_isEditMode;
    
    wxDECLARE_EVENT_TABLE();
};
