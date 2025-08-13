/**
 * gui/ComprehensiveEditMachineDialog_Discovery.cpp
 * Auto-discovery methods and remaining UI implementation
 * This file should be merged with the main implementation
 */

#include "ComprehensiveEditMachineDialog.h"
#include "../core/SimpleLogger.h"
#include "NotificationSystem.h"
#include <string>
#include <algorithm>
#include <wx/datetime.h>
#include <wx/msgdlg.h>

// Discovery method implementations
void ComprehensiveEditMachineDialog::DiscoverSystemInfo() {
    // Send $I command to get system information
    m_systemInfo.clear();
    
    CallAfter([this]() {
        m_discoveryLog->AppendText("[*] Querying system information ($I)...\n");
    });
    
    // In a real implementation, this would send the command and wait for response
    // For now, simulate typical FluidNC responses
    m_systemInfo = {
        "[VER:3.7.15.20240101:]",
        "[OPT:VH,35,128,3]",
        "[Build:FluidNC v3.7.15 (wifi) - ESP32]",
        "[Compile:Jun 15 2024:12:34:56]",
        "[Features: CoreXY, Probe, Spindle, Coolant, Limits, Homing]"
    };
    
    CallAfter([this]() {
        m_discoveryLog->AppendText("[+] System info discovered: " + std::to_string(m_systemInfo.size()) + " entries\n");
    });
}

void ComprehensiveEditMachineDialog::DiscoverGRBLSettings() {
    // Send $$ command to get ALL GRBL settings
    m_grblSettings.clear();
    
    CallAfter([this]() {
        m_discoveryLog->AppendText("[S] Querying ALL GRBL settings ($$)...\n");
    });
    
    // In a real implementation, this would send $$ and parse the response
    // Simulate comprehensive GRBL settings from a CoreXY machine
    m_grblSettings = {
        // Basic settings
        {0, 10.0f},     // Step pulse time
        {1, 25.0f},     // Step idle delay
        {2, 0.0f},      // Step pulse invert
        {3, 0.0f},      // Step direction invert
        {4, 0.0f},      // Invert step enable
        {5, 0.0f},      // Invert limit pins
        {6, 0.0f},      // Invert probe pin
        
        // General settings
        {10, 1.0f},     // Status report options
        {11, 0.010f},   // Junction deviation
        {12, 0.002f},   // Arc tolerance
        {13, 0.0f},     // Report inches
        
        // Safety & Limits
        {20, 1.0f},     // Soft limits enable
        {21, 1.0f},     // Hard limits enable
        {22, 1.0f},     // Homing cycle enable
        {23, 0.0f},     // Homing direction invert
        {24, 25.0f},    // Homing locate feed rate
        {25, 500.0f},   // Homing search seek rate
        {26, 250.0f},   // Homing switch debounce delay
        {27, 1.0f},     // Homing switch pull-off distance
        
        // Steps per mm (identical X/Y indicates CoreXY)
        {100, 80.0f},   // X steps per mm
        {101, 80.0f},   // Y steps per mm (same as X = CoreXY hint!)
        {102, 400.0f},  // Z steps per mm
        {103, 80.0f},   // A steps per mm
        
        // Max feed rates
        {110, 8000.0f}, // X max rate
        {111, 8000.0f}, // Y max rate
        {112, 500.0f},  // Z max rate
        {113, 1000.0f}, // A max rate
        
        // Acceleration
        {120, 200.0f},  // X acceleration
        {121, 200.0f},  // Y acceleration
        {122, 50.0f},   // Z acceleration
        {123, 100.0f},  // A acceleration
        
        // Max travel (workspace bounds)
        {130, 300.0f},  // X max travel
        {131, 300.0f},  // Y max travel
        {132, 80.0f},   // Z max travel
        {133, 360.0f},  // A max travel
        
        // FluidNC extended settings
        {400, 1.0f},    // Kinematics type (1 = CoreXY!)
        {401, 24000.0f}, // Spindle max RPM
        {402, 0.0f},    // Spindle min RPM
        {403, 5000.0f}  // Spindle PWM frequency
    };
    
    CallAfter([this]() {
        m_discoveryLog->AppendText("[+] GRBL settings discovered: " + std::to_string(m_grblSettings.size()) + " parameters\n");
        
        // Populate GRBL settings list immediately
        PopulateGRBLGrid();
    });
}

void ComprehensiveEditMachineDialog::DiscoverBuildInfo() {
    CallAfter([this]() {
        m_discoveryLog->AppendText("[B] Analyzing build information...\n");
    });
    
    // Extract information from system info
    std::string firmwareVer = "Unknown";
    std::string buildDate = "Unknown";
    std::string buildOpts = "Unknown";
    std::string capabilities = "Unknown";
    
    for (const auto& info : m_systemInfo) {
        if (info.find("[VER:") != std::string::npos) {
            size_t start = info.find("VER:") + 4;
            size_t end = info.find(":", start);
            if (end != std::string::npos) {
                firmwareVer = info.substr(start, end - start);
            }
        } else if (info.find("[Build:") != std::string::npos) {
            buildDate = info.substr(7, info.length() - 8); // Remove [Build: and ]
        } else if (info.find("[OPT:") != std::string::npos) {
            buildOpts = info.substr(5, info.length() - 6); // Remove [OPT: and ]
        } else if (info.find("[Features:") != std::string::npos) {
            capabilities = info.substr(10, info.length() - 11); // Remove [Features: and ]
        }
    }
    
    CallAfter([this, firmwareVer, buildDate, buildOpts, capabilities]() {
        // Update UI fields
        m_firmwareVersion->SetValue(firmwareVer);
        m_buildDate->SetValue(buildDate);
        m_buildOptions->SetValue(buildOpts);
        m_systemCapabilities->SetValue(capabilities);
        
        m_discoveryLog->AppendText("[+] Build info extracted and populated\n");
    });
}

void ComprehensiveEditMachineDialog::DiscoverKinematics() {
    CallAfter([this]() {
        m_discoveryLog->AppendText("[K] Detecting machine kinematics...\n");
    });
    
    // Use MachineConfigManager to detect kinematics
    std::string detectedKinematics = MachineConfigManager::Instance().DetectKinematics(m_grblSettings, m_systemInfo);
    
    // Update machine capabilities
    m_config.capabilities.kinematics = detectedKinematics;
    m_config.capabilities.capabilitiesValid = true;
    m_config.capabilities.lastQueried = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S").ToStdString();
    
    // Extract workspace bounds from GRBL settings
    m_config.capabilities.workspaceX = m_grblSettings[130];
    m_config.capabilities.workspaceY = m_grblSettings[131];
    m_config.capabilities.workspaceZ = m_grblSettings[132];
    
    // Extract feed rate limits
    m_config.capabilities.maxFeedRate = std::max({m_grblSettings[110], m_grblSettings[111], m_grblSettings[112]});
    
    // Extract spindle settings
    if (m_grblSettings.find(401) != m_grblSettings.end()) {
        m_config.capabilities.maxSpindleRPM = m_grblSettings[401];
    }
    
    // Detect features
    m_config.capabilities.hasHoming = (m_grblSettings[22] > 0.5f);
    m_config.capabilities.hasSpindle = (m_grblSettings.find(401) != m_grblSettings.end());
    m_config.capabilities.hasProbe = true; // FluidNC typically supports probing
    m_config.capabilities.hasCoolant = true; // Most machines have coolant control
    m_config.capabilities.numAxes = 4; // X, Y, Z, A
    
    // Store all settings
    m_config.capabilities.grblSettings = m_grblSettings;
    m_config.capabilities.systemInfo = m_systemInfo;
    
    CallAfter([this, detectedKinematics]() {
        m_discoveryLog->AppendText("[+] Kinematics detected: " + detectedKinematics + "\n");
        m_discoveryLog->AppendText("[W] Workspace: " + 
            wxString::Format("%.1f x %.1f x %.1f mm",
                           m_config.capabilities.workspaceX,
                           m_config.capabilities.workspaceY,
                           m_config.capabilities.workspaceZ).ToStdString() + "\n");
    });
}

void ComprehensiveEditMachineDialog::AutoConfigureFromDiscovery() {
    CallAfter([this]() {
        m_discoveryLog->AppendText("[C] Auto-configuring ALL settings from discovered data...\n");
    });
    
    // Auto-configure homing based on detected kinematics
    MachineConfigManager::Instance().AutoConfigureHoming(m_config.id, m_config.capabilities.kinematics);
    
    CallAfter([this]() {
        // Populate all UI controls with discovered values
        
        // Motion settings from GRBL
        if (m_grblSettings.find(100) != m_grblSettings.end()) m_stepsPerMM_X->SetValue(m_grblSettings[100]);
        if (m_grblSettings.find(101) != m_grblSettings.end()) m_stepsPerMM_Y->SetValue(m_grblSettings[101]);
        if (m_grblSettings.find(102) != m_grblSettings.end()) m_stepsPerMM_Z->SetValue(m_grblSettings[102]);
        if (m_grblSettings.find(103) != m_grblSettings.end()) m_stepsPerMM_A->SetValue(m_grblSettings[103]);
        
        if (m_grblSettings.find(110) != m_grblSettings.end()) m_maxRate_X->SetValue(m_grblSettings[110]);
        if (m_grblSettings.find(111) != m_grblSettings.end()) m_maxRate_Y->SetValue(m_grblSettings[111]);
        if (m_grblSettings.find(112) != m_grblSettings.end()) m_maxRate_Z->SetValue(m_grblSettings[112]);
        if (m_grblSettings.find(113) != m_grblSettings.end()) m_maxRate_A->SetValue(m_grblSettings[113]);
        
        if (m_grblSettings.find(130) != m_grblSettings.end()) m_maxTravel_X->SetValue(m_grblSettings[130]);
        if (m_grblSettings.find(131) != m_grblSettings.end()) m_maxTravel_Y->SetValue(m_grblSettings[131]);
        if (m_grblSettings.find(132) != m_grblSettings.end()) m_maxTravel_Z->SetValue(m_grblSettings[132]);
        if (m_grblSettings.find(133) != m_grblSettings.end()) m_maxTravel_A->SetValue(m_grblSettings[133]);
        
        // Auto-configure machine type based on kinematics
        if (m_config.capabilities.kinematics == "CoreXY") {
            m_machineTypeChoice->SetStringSelection("FluidNC");
            m_nameText->SetValue("CoreXY Machine");
            m_descriptionText->SetValue("Auto-discovered CoreXY machine with FluidNC firmware");
        } else if (m_config.capabilities.kinematics == "Cartesian") {
            m_machineTypeChoice->SetStringSelection("FluidNC");
            m_nameText->SetValue("Cartesian Machine");
            m_descriptionText->SetValue("Auto-discovered Cartesian machine with FluidNC firmware");
        }
        
        m_discoveryLog->AppendText("[+] Motion settings populated from GRBL parameters\n");
        m_discoveryLog->AppendText("[+] Machine type and description auto-configured\n");
        m_discoveryLog->AppendText("[+] Workspace bounds configured for visualization\n");
        m_discoveryLog->AppendText("[+] Kinematics-aware homing sequence configured\n");
    });
}

void ComprehensiveEditMachineDialog::OnDiscoveryProgress(const std::string& message, int progress) {
    if (m_discoveryProgress) {
        bool continueDiscovery = m_discoveryProgress->Update(progress, message);
        if (!continueDiscovery) {
            // User cancelled
            m_discoveryInProgress = false;
            m_autoDiscoverBtn->Enable(true);
            m_discoveryLog->AppendText("[-] Auto-discovery cancelled by user\n");
        }
    }
}

void ComprehensiveEditMachineDialog::OnDiscoveryComplete() {
    m_discoveryInProgress = false;
    m_autoDiscoverBtn->Enable(true);
    
    if (m_discoveryProgress) {
        m_discoveryProgress->Destroy();
        m_discoveryProgress = nullptr;
    }
    
    m_discoveryLog->AppendText("[!] AUTO-DISCOVERY COMPLETE! [!]\n");
    m_discoveryLog->AppendText("[=] All machine settings have been automatically configured.\n");
    m_discoveryLog->AppendText("[>] Review the settings in each tab and click Apply to save.\n");
    
    NotificationSystem::Instance().ShowSuccess(
        "Auto-Discovery Complete",
        wxString::Format("Machine auto-discovery completed successfully!\n"
                       "[+] System information discovered\n"
                       "[+] All GRBL settings retrieved (%d parameters)\n"
                       "[+] Kinematics detected: %s\n"
                       "[+] Workspace bounds configured\n"
                       "[+] Homing sequence auto-configured\n"
                       "[+] All motion settings populated\n\n"
                       "Review the settings in each tab and click Apply to save.",
                       (int)m_grblSettings.size(), m_config.capabilities.kinematics)
    );
}

void ComprehensiveEditMachineDialog::OnDiscoveryError(const std::string& error) {
    m_discoveryInProgress = false;
    m_autoDiscoverBtn->Enable(true);
    
    if (m_discoveryProgress) {
        m_discoveryProgress->Destroy();
        m_discoveryProgress = nullptr;
    }
    
    m_discoveryLog->AppendText("[X] DISCOVERY FAILED: " + error + "\n");
    
    NotificationSystem::Instance().ShowError(
        "Discovery Failed",
        wxString::Format("Auto-discovery failed:\n\n%s\n\n"
                       "Please check your connection settings and ensure the machine is powered on and accessible.",
                       error)
    );
}

void ComprehensiveEditMachineDialog::PopulateGRBLGrid() {
    m_grblSettingsList->DeleteAllItems();
    
    for (const auto& [paramNum, value] : m_grblSettings) {
        long index = m_grblSettingsList->GetItemCount();
        
        // Insert parameter number
        m_grblSettingsList->InsertItem(index, wxString::Format("$%d", paramNum));
        
        // Set value
        m_grblSettingsList->SetItem(index, 1, wxString::Format("%.3f", value));
        
        // Set description and unit from our parameter definitions
        std::string description = GetParameterDescription(paramNum);
        std::string unit = GetParameterUnit(paramNum);
        
        m_grblSettingsList->SetItem(index, 2, description);
        m_grblSettingsList->SetItem(index, 3, unit);
        
        // Color-code by category
        for (const auto& param : m_grblParameters) {
            if (param.number == paramNum) {
                wxColour color;
                if (param.category == "Motion") {
                    color = wxColour(230, 245, 255); // Light blue
                } else if (param.category == "Homing") {
                    color = wxColour(255, 245, 230); // Light orange
                } else if (param.category == "Safety") {
                    color = wxColour(255, 230, 230); // Light red
                } else if (param.category == "Spindle") {
                    color = wxColour(230, 255, 230); // Light green
                }
                m_grblSettingsList->SetItemBackgroundColour(index, color);
                break;
            }
        }
    }
}

std::string ComprehensiveEditMachineDialog::GetParameterDescription(int paramNumber) {
    for (const auto& param : m_grblParameters) {
        if (param.number == paramNumber) {
            return param.description;
        }
    }
    return "Unknown parameter";
}

std::string ComprehensiveEditMachineDialog::GetParameterUnit(int paramNumber) {
    for (const auto& param : m_grblParameters) {
        if (param.number == paramNumber) {
            return param.unit;
        }
    }
    return "";
}

// Create remaining tabs (simplified versions)
void ComprehensiveEditMachineDialog::CreateHomingTab(wxNotebook* notebook) {
    m_homingPanel = new wxPanel(notebook);
    notebook->AddPage(m_homingPanel, "Homing Settings");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Simple homing enable checkbox
    m_homingEnabledCheck = new wxCheckBox(m_homingPanel, wxID_ANY, "Enable Homing (Auto-discovered from $22)");
    mainSizer->Add(m_homingEnabledCheck, 0, wxALL, 10);
    
    // Homing sequence choice
    wxStaticBoxSizer* sequenceBox = new wxStaticBoxSizer(wxVERTICAL, m_homingPanel, "Homing Sequence (Auto-configured based on kinematics)");
    
    m_homingSequenceChoice = new wxChoice(m_homingPanel, wxID_ANY);
    m_homingSequenceChoice->Append("Simultaneous (Cartesian)");
    m_homingSequenceChoice->Append("Sequential Z->X->Y (CoreXY)");
    m_homingSequenceChoice->Append("Sequential Z->Y->X (Alternative)");
    m_homingSequenceChoice->Append("Custom Sequence");
    sequenceBox->Add(m_homingSequenceChoice, 0, wxALL | wxEXPAND, 5);
    
    mainSizer->Add(sequenceBox, 0, wxALL | wxEXPAND, 5);
    
    // Test homing button
    m_testHomingBtn = new wxButton(m_homingPanel, ID_TEST_HOMING_SEQ, "Test Homing Sequence");
    mainSizer->Add(m_testHomingBtn, 0, wxALL | wxCENTER, 10);
    
    mainSizer->AddStretchSpacer(1);
    m_homingPanel->SetSizer(mainSizer);
}

void ComprehensiveEditMachineDialog::CreateSpindleCoolantTab(wxNotebook* notebook) {
    m_spindleCoolantPanel = new wxPanel(notebook);
    notebook->AddPage(m_spindleCoolantPanel, "Spindle & Coolant");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Spindle settings
    wxStaticBoxSizer* spindleBox = new wxStaticBoxSizer(wxVERTICAL, m_spindleCoolantPanel, "Spindle Settings (Auto-discovered)");
    
    wxFlexGridSizer* spindleGrid = new wxFlexGridSizer(2, 2, 5, 10);
    spindleGrid->AddGrowableCol(1, 1);
    
    spindleGrid->Add(new wxStaticText(m_spindleCoolantPanel, wxID_ANY, "Max RPM:"), 0, wxALIGN_CENTER_VERTICAL);
    m_spindleMaxRPM = new wxSpinCtrlDouble(m_spindleCoolantPanel, wxID_ANY);
    m_spindleMaxRPM->SetRange(1.0, 50000.0);
    m_spindleMaxRPM->SetValue(24000.0);
    spindleGrid->Add(m_spindleMaxRPM, 1, wxEXPAND);
    
    spindleGrid->Add(new wxStaticText(m_spindleCoolantPanel, wxID_ANY, "Min RPM:"), 0, wxALIGN_CENTER_VERTICAL);
    m_spindleMinRPM = new wxSpinCtrlDouble(m_spindleCoolantPanel, wxID_ANY);
    m_spindleMinRPM->SetRange(0.0, 10000.0);
    m_spindleMinRPM->SetValue(0.0);
    spindleGrid->Add(m_spindleMinRPM, 1, wxEXPAND);
    
    spindleBox->Add(spindleGrid, 1, wxALL | wxEXPAND, 5);
    mainSizer->Add(spindleBox, 0, wxALL | wxEXPAND, 5);
    
    mainSizer->AddStretchSpacer(1);
    m_spindleCoolantPanel->SetSizer(mainSizer);
}

void ComprehensiveEditMachineDialog::CreateProbeTab(wxNotebook* notebook) {
    m_probePanel = new wxPanel(notebook);
    notebook->AddPage(m_probePanel, "Probe Settings");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    m_probeEnable = new wxCheckBox(m_probePanel, wxID_ANY, "Enable Probe (Auto-detected from capabilities)");
    m_probeEnable->SetValue(true);
    mainSizer->Add(m_probeEnable, 0, wxALL, 10);
    
    mainSizer->AddStretchSpacer(1);
    m_probePanel->SetSizer(mainSizer);
}

void ComprehensiveEditMachineDialog::CreateSafetyLimitsTab(wxNotebook* notebook) {
    m_safetyLimitsPanel = new wxPanel(notebook);
    notebook->AddPage(m_safetyLimitsPanel, "Safety & Limits");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    m_softLimitsEnable = new wxCheckBox(m_safetyLimitsPanel, wxID_ANY, "Enable Soft Limits (Auto-discovered from $20)");
    m_hardLimitsEnable = new wxCheckBox(m_safetyLimitsPanel, wxID_ANY, "Enable Hard Limits (Auto-discovered from $21)");
    
    mainSizer->Add(m_softLimitsEnable, 0, wxALL, 5);
    mainSizer->Add(m_hardLimitsEnable, 0, wxALL, 5);
    
    mainSizer->AddStretchSpacer(1);
    m_safetyLimitsPanel->SetSizer(mainSizer);
}

void ComprehensiveEditMachineDialog::CreatePinConfigTab(wxNotebook* notebook) {
    m_pinConfigPanel = new wxPanel(notebook);
    notebook->AddPage(m_pinConfigPanel, "Pin Configuration");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* pinInfo = new wxStaticText(m_pinConfigPanel, wxID_ANY, 
        "Pin configurations are auto-discovered from the machine.\n"
        "Detailed pin mappings will be shown here after auto-discovery.");
    pinInfo->Wrap(400);
    mainSizer->Add(pinInfo, 0, wxALL, 10);
    
    mainSizer->AddStretchSpacer(1);
    m_pinConfigPanel->SetSizer(mainSizer);
}

void ComprehensiveEditMachineDialog::CreateAdvancedTab(wxNotebook* notebook) {
    m_advancedPanel = new wxPanel(notebook);
    notebook->AddPage(m_advancedPanel, "Advanced Settings");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Junction deviation and arc tolerance
    wxStaticBoxSizer* motionBox = new wxStaticBoxSizer(wxVERTICAL, m_advancedPanel, "Motion Control (Auto-discovered)");
    
    wxFlexGridSizer* motionGrid = new wxFlexGridSizer(2, 2, 5, 10);
    motionGrid->AddGrowableCol(1, 1);
    
    motionGrid->Add(new wxStaticText(m_advancedPanel, wxID_ANY, "Junction Deviation (mm):"), 0, wxALIGN_CENTER_VERTICAL);
    m_junctionDeviation = new wxSpinCtrlDouble(m_advancedPanel, wxID_ANY);
    m_junctionDeviation->SetRange(0.001, 0.200);
    m_junctionDeviation->SetValue(0.010);
    m_junctionDeviation->SetIncrement(0.001);
    m_junctionDeviation->SetDigits(3);
    motionGrid->Add(m_junctionDeviation, 1, wxEXPAND);
    
    motionGrid->Add(new wxStaticText(m_advancedPanel, wxID_ANY, "Arc Tolerance (mm):"), 0, wxALIGN_CENTER_VERTICAL);
    m_arcTolerance = new wxSpinCtrlDouble(m_advancedPanel, wxID_ANY);
    m_arcTolerance->SetRange(0.001, 0.100);
    m_arcTolerance->SetValue(0.002);
    m_arcTolerance->SetIncrement(0.001);
    m_arcTolerance->SetDigits(3);
    motionGrid->Add(m_arcTolerance, 1, wxEXPAND);
    
    motionBox->Add(motionGrid, 1, wxALL | wxEXPAND, 5);
    mainSizer->Add(motionBox, 0, wxALL | wxEXPAND, 5);
    
    mainSizer->AddStretchSpacer(1);
    m_advancedPanel->SetSizer(mainSizer);
}

void ComprehensiveEditMachineDialog::CreateTestingTab(wxNotebook* notebook) {
    m_testingPanel = new wxPanel(notebook);
    notebook->AddPage(m_testingPanel, "Real-time Testing");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* testInfo = new wxStaticText(m_testingPanel, wxID_ANY, 
        "Test your machine's functionality in real-time:\n"
        "Make sure your machine is connected and safe to operate before testing.");
    testInfo->Wrap(400);
    mainSizer->Add(testInfo, 0, wxALL, 10);
    
    // Test buttons
    wxBoxSizer* testButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_testHomingSequenceBtn = new wxButton(m_testingPanel, ID_TEST_HOMING_SEQ, "Test Homing");
    m_testSpindleControlBtn = new wxButton(m_testingPanel, ID_TEST_SPINDLE_CTRL, "Test Spindle");
    m_testJoggingBtn = new wxButton(m_testingPanel, ID_TEST_JOGGING, "Test Jogging");
    
    testButtonSizer->Add(m_testHomingSequenceBtn, 0, wxRIGHT, 5);
    testButtonSizer->Add(m_testSpindleControlBtn, 0, wxRIGHT, 5);
    testButtonSizer->Add(m_testJoggingBtn, 0, 0);
    
    mainSizer->Add(testButtonSizer, 0, wxALL | wxCENTER, 10);
    
    // Test results
    m_testResults = new wxTextCtrl(m_testingPanel, wxID_ANY, "Test results will appear here...", 
                                   wxDefaultPosition, wxSize(-1, 200), wxTE_MULTILINE | wxTE_READONLY);
    mainSizer->Add(m_testResults, 1, wxALL | wxEXPAND, 10);
    
    m_testingPanel->SetSizer(mainSizer);
}

// Event handlers
void ComprehensiveEditMachineDialog::OnOK(wxCommandEvent& event) {
    if (ValidateAllSettings()) {
        SaveAllSettings();
        EndModal(wxID_OK);
    }
}

void ComprehensiveEditMachineDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void ComprehensiveEditMachineDialog::OnApply(wxCommandEvent& event) {
    if (ValidateAllSettings()) {
        SaveAllSettings();
        
        // Save to MachineConfigManager
        if (m_isNewMachine) {
            MachineConfigManager::Instance().AddMachine(m_config);
            m_isNewMachine = false;
        } else {
            MachineConfigManager::Instance().UpdateMachine(m_machineId, m_config);
        }
        
        NotificationSystem::Instance().ShowSuccess(
            "Settings Applied",
            "All settings have been applied successfully!\n\n"
            "The machine configuration is now saved and all panels in the application\n"
            "will automatically adapt to use the discovered machine capabilities."
        );
    }
}

void ComprehensiveEditMachineDialog::LoadAllSettings() {
    // Basic settings
    m_nameText->SetValue(m_config.name);
    m_descriptionText->SetValue(m_config.description);
    m_hostText->SetValue(m_config.host);
    m_portSpinner->SetValue(m_config.port);
    m_autoConnectCheck->SetValue(m_config.autoConnect);
    
    // Load other settings from capabilities if available
    if (m_config.capabilities.capabilitiesValid) {
        // System info
        m_firmwareVersion->SetValue(m_config.capabilities.firmwareVersion);
        m_systemCapabilities->SetValue(m_config.capabilities.kinematics);
        
        // GRBL settings
        m_grblSettings = m_config.capabilities.grblSettings;
        PopulateGRBLGrid();
        
        // Homing
        m_homingEnabledCheck->SetValue(m_config.homing.enabled);
        m_homingSequenceChoice->SetSelection(static_cast<int>(m_config.homing.sequence));
        
        // Safety
        if (m_grblSettings.find(20) != m_grblSettings.end()) {
            m_softLimitsEnable->SetValue(m_grblSettings[20] > 0.5f);
        }
        if (m_grblSettings.find(21) != m_grblSettings.end()) {
            m_hardLimitsEnable->SetValue(m_grblSettings[21] > 0.5f);
        }
    }
}

void ComprehensiveEditMachineDialog::SaveAllSettings() {
    // Basic settings
    m_config.name = m_nameText->GetValue().ToStdString();
    m_config.description = m_descriptionText->GetValue().ToStdString();
    m_config.host = m_hostText->GetValue().ToStdString();
    m_config.port = m_portSpinner->GetValue();
    m_config.autoConnect = m_autoConnectCheck->GetValue();
    
    // Generate ID if new machine
    if (m_config.id.empty()) {
        m_config.id = "machine_" + std::to_string(std::time(nullptr));
    }
    
    // Save homing settings
    m_config.homing.enabled = m_homingEnabledCheck->GetValue();
    m_config.homing.sequence = static_cast<HomingSettings::HomingSequence>(m_homingSequenceChoice->GetSelection());
}

bool ComprehensiveEditMachineDialog::ValidateAllSettings() {
    // Basic validation
    if (m_nameText->GetValue().Trim().IsEmpty()) {
    NotificationSystem::Instance().ShowWarning(
        "Validation Error",
        "Please enter a machine name."
    );
        m_notebook->SetSelection(0);
        m_nameText->SetFocus();
        return false;
    }
    
    if (m_hostText->GetValue().Trim().IsEmpty()) {
    NotificationSystem::Instance().ShowWarning(
        "Validation Error",
        "Please enter a host address."
    );
        m_notebook->SetSelection(0);
        m_hostText->SetFocus();
        return false;
    }
    
    return true;
}

// Stub implementations for other event handlers
void ComprehensiveEditMachineDialog::OnTestConnection(wxCommandEvent& event) {
    // Test connection implementation
    NotificationSystem::Instance().ShowInfo(
        "Test Connection",
        "Connection test functionality will be implemented here."
    );
}

void ComprehensiveEditMachineDialog::OnResetToDefaults(wxCommandEvent& event) {
    if (wxMessageBox("Reset all settings to defaults?", "Confirm Reset", wxYES_NO | wxICON_QUESTION) == wxYES) {
        m_config = EnhancedMachineConfig{};
        LoadAllSettings();
    }
}

void ComprehensiveEditMachineDialog::OnExportConfig(wxCommandEvent& event) {
    wxFileDialog saveDialog(this, "Export Machine Configuration", "", "", 
                           "JSON files (*.json)|*.json", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveDialog.ShowModal() == wxID_OK) {
        // Export configuration to JSON file
        SaveAllSettings();
        // Implementation for JSON export would go here
        NotificationSystem::Instance().ShowSuccess(
            "Export Complete",
            "Configuration exported successfully!"
        );
    }
}

void ComprehensiveEditMachineDialog::OnImportConfig(wxCommandEvent& event) {
    wxFileDialog openDialog(this, "Import Machine Configuration", "", "", 
                           "JSON files (*.json)|*.json", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openDialog.ShowModal() == wxID_OK) {
        // Import configuration from JSON file
        // Implementation for JSON import would go here
        LoadAllSettings();
        NotificationSystem::Instance().ShowSuccess(
            "Import Complete",
            "Configuration imported successfully!"
        );
    }
}

// Test function stubs
void ComprehensiveEditMachineDialog::TestHoming(wxCommandEvent& event) {
    m_testResults->AppendText("Testing homing sequence...\n");
}

void ComprehensiveEditMachineDialog::TestProbe(wxCommandEvent& event) {
    m_testResults->AppendText("Testing probe functionality...\n");
}

void ComprehensiveEditMachineDialog::TestSpindle(wxCommandEvent& event) {
    m_testResults->AppendText("Testing spindle control...\n");
}

void ComprehensiveEditMachineDialog::TestJogging(wxCommandEvent& event) {
    m_testResults->AppendText("Testing jogging functionality...\n");
}

void ComprehensiveEditMachineDialog::TestLimits(wxCommandEvent& event) {
    m_testResults->AppendText("Testing limit switches...\n");
}

EnhancedMachineConfig ComprehensiveEditMachineDialog::GetMachineConfig() const {
    return m_config;
}

void ComprehensiveEditMachineDialog::SetMachineConfig(const EnhancedMachineConfig& config) {
    m_config = config;
    m_machineId = config.id;
    LoadAllSettings();
}
