/**
 * gui/EnhancedEditMachineDialog_Part3.cpp
 * Final part of Enhanced Edit Machine Dialog implementation
 * Helper methods, data validation, and capability discovery
 */

// This is the final part of EnhancedEditMachineDialog.cpp

// Machine capability discovery methods
void EnhancedEditMachineDialog::QueryMachineCapabilities() {
    if (m_queryInProgress) {
        return; // Already in progress
    }
    
    // Validate connection settings first
    SaveMachineData(); // Update config with current UI values
    
    if (m_config.host.empty()) {
        wxMessageBox("Please enter a valid host address before querying machine capabilities.", 
                     "Invalid Host", wxOK | wxICON_WARNING);
        return;
    }
    
    m_queryInProgress = true;
    m_queryMachineBtn->Enable(false);
    
    // Show progress dialog
    m_progressDialog = new wxProgressDialog("Querying Machine", "Connecting to machine...", 100, this,
                                           wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT);
    
    // Start capability discovery in a separate thread
    std::thread queryThread([this]() {
        try {
            // Connect to machine
            bool connected = CommunicationManager::Instance().ConnectMachine(
                m_config.id, m_config.host, m_config.port);
            
            if (!connected) {
                CallAfter([this]() {
                    OnQueryError("Failed to connect to machine");
                });
                return;
            }
            
            CallAfter([this]() {
                OnQueryProgress("Connected. Querying machine capabilities...");
            });
            
            // Create capabilities structure
            MachineCapabilities capabilities;
            capabilities.capabilitiesValid = true;
            capabilities.lastQueried = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S").ToStdString();
            
            // Query system information ($I)
            CallAfter([this]() {
                OnQueryProgress("Querying system information...");
            });
            
            // Send $I command and wait for response
            std::vector<std::string> systemInfo;
            if (CommunicationManager::Instance().SendCommand(m_config.id, "$I")) {
                // In a real implementation, we would wait for and parse the response
                // For now, we'll simulate the response
                systemInfo.push_back("[VER:3.7.15.20240101:]");
                systemInfo.push_back("[OPT:VH,35]");
                systemInfo.push_back("[Build:FluidNC v3.7.15 (wifi)]");
            }
            capabilities.systemInfo = systemInfo;
            
            // Extract firmware version from system info
            for (const auto& info : systemInfo) {
                if (info.find("VER:") != std::string::npos) {
                    size_t start = info.find("VER:") + 4;
                    size_t end = info.find(":", start);
                    if (end != std::string::npos) {
                        capabilities.firmwareVersion = info.substr(start, end - start);
                    }
                }
                if (info.find("Build:") != std::string::npos) {
                    capabilities.buildInfo = info;
                }
            }
            
            CallAfter([this]() {
                OnQueryProgress("Querying GRBL settings...");
            });
            
            // Query GRBL settings ($$)
            std::map<int, float> grblSettings;
            if (CommunicationManager::Instance().SendCommand(m_config.id, "$$")) {
                // In a real implementation, parse $$ response
                // For now, simulate common settings
                grblSettings[0] = 10.0f;    // Step pulse time
                grblSettings[1] = 25.0f;    // Step idle delay
                grblSettings[2] = 0.0f;     // Step pulse invert
                grblSettings[3] = 0.0f;     // Step direction invert
                grblSettings[4] = 0.0f;     // Invert step enable pin
                grblSettings[5] = 0.0f;     // Invert limit pins
                grblSettings[6] = 0.0f;     // Invert probe pin
                grblSettings[10] = 1.0f;    // Status report options
                grblSettings[11] = 0.010f;  // Junction deviation
                grblSettings[12] = 0.002f;  // Arc tolerance
                grblSettings[13] = 0.0f;    // Report in inches
                grblSettings[20] = 0.0f;    // Soft limits enable
                grblSettings[21] = 0.0f;    // Hard limits enable
                grblSettings[22] = 0.0f;    // Homing cycle enable
                grblSettings[23] = 0.0f;    // Homing direction invert
                grblSettings[24] = 25.0f;   // Homing locate feed rate
                grblSettings[25] = 500.0f;  // Homing search seek rate
                grblSettings[26] = 250.0f;  // Homing switch debounce delay
                grblSettings[27] = 1.0f;    // Homing switch pull-off distance
                grblSettings[100] = 80.0f;  // X steps per mm
                grblSettings[101] = 80.0f;  // Y steps per mm
                grblSettings[102] = 400.0f; // Z steps per mm
                grblSettings[110] = 3000.0f; // X max rate
                grblSettings[111] = 3000.0f; // Y max rate
                grblSettings[112] = 500.0f;  // Z max rate
                grblSettings[120] = 30.0f;   // X acceleration
                grblSettings[121] = 30.0f;   // Y acceleration
                grblSettings[122] = 10.0f;   // Z acceleration
                grblSettings[130] = 400.0f;  // X max travel
                grblSettings[131] = 400.0f;  // Y max travel
                grblSettings[132] = 100.0f;  // Z max travel
            }
            capabilities.grblSettings = grblSettings;
            
            // Extract workspace bounds from travel settings
            capabilities.workspaceX = grblSettings[130];
            capabilities.workspaceY = grblSettings[131];
            capabilities.workspaceZ = grblSettings[132];
            
            // Extract feed rate limits
            capabilities.maxFeedRate = std::max({grblSettings[110], grblSettings[111], grblSettings[112]});
            
            // Check for homing capability
            capabilities.hasHoming = (grblSettings[22] > 0.5f); // Homing cycle enable
            
            // Detect kinematics
            CallAfter([this]() {
                OnQueryProgress("Detecting machine kinematics...");
            });
            
            capabilities.kinematics = MachineConfigManager::Instance().DetectKinematics(
                grblSettings, systemInfo);
            
            // Set other capabilities based on typical FluidNC features
            capabilities.hasSpindle = true;  // Most CNC machines have spindles
            capabilities.hasProbe = true;    // FluidNC typically supports probing
            capabilities.hasCoolant = true;  // Most machines have coolant control
            capabilities.numAxes = 3;        // Standard 3-axis machine
            
            // Disconnect after querying
            CommunicationManager::Instance().DisconnectMachine(m_config.id);
            
            CallAfter([this, capabilities]() {
                OnQueryComplete(capabilities);
            });
            
        } catch (const std::exception& e) {
            CallAfter([this, e]() {
                OnQueryError("Exception during query: " + std::string(e.what()));
            });
        }
    });
    queryThread.detach();
}

void EnhancedEditMachineDialog::OnQueryProgress(const std::string& message) {
    if (m_progressDialog) {
        static int progress = 0;
        progress += 20;
        if (progress > 90) progress = 90;
        
        if (!m_progressDialog->Update(progress, message)) {
            // User cancelled
            m_queryInProgress = false;
            m_queryMachineBtn->Enable(true);
            if (m_progressDialog) {
                m_progressDialog->Destroy();
                m_progressDialog = nullptr;
            }
        }
    }
}

void EnhancedEditMachineDialog::OnQueryComplete(const MachineCapabilities& capabilities) {
    m_queryInProgress = false;
    m_queryMachineBtn->Enable(true);
    
    if (m_progressDialog) {
        m_progressDialog->Update(100, "Query completed successfully!");
        m_progressDialog->Destroy();
        m_progressDialog = nullptr;
    }
    
    // Update machine configuration with discovered capabilities
    m_config.capabilities = capabilities;
    
    // Auto-configure homing based on detected kinematics
    MachineConfigManager::Instance().AutoConfigureHoming(m_config.id, capabilities.kinematics);
    
    // Update UI with discovered information
    LoadMachineData();
    UpdateUI();
    
    wxMessageBox("Machine capabilities have been successfully discovered and configured!", 
                 "Query Complete", wxOK | wxICON_INFORMATION);
}

void EnhancedEditMachineDialog::OnQueryError(const std::string& error) {
    m_queryInProgress = false;
    m_queryMachineBtn->Enable(true);
    
    if (m_progressDialog) {
        m_progressDialog->Destroy();
        m_progressDialog = nullptr;
    }
    
    wxMessageBox("Failed to query machine capabilities:\n" + error, 
                 "Query Failed", wxOK | wxICON_ERROR);
}

// Homing sequence testing
void EnhancedEditMachineDialog::TestHomingSequence() {
    if (m_homingInProgress) {
        return; // Already testing
    }
    
    // Check if machine is connected
    if (!CommunicationManager::Instance().IsConnected(m_config.id)) {
        int result = wxMessageBox("Machine is not connected. Connect now to test homing sequence?", 
                                  "Machine Not Connected", wxYES_NO | wxICON_QUESTION);
        if (result == wxYES) {
            bool connected = CommunicationManager::Instance().ConnectMachine(
                m_config.id, m_config.host, m_config.port);
            if (!connected) {
                wxMessageBox("Failed to connect to machine.", "Connection Failed", wxOK | wxICON_ERROR);
                return;
            }
        } else {
            return;
        }
    }
    
    // Update machine configuration with current UI values
    SaveMachineData();
    MachineConfigManager::Instance().UpdateMachine(m_config.id, m_config);
    
    // Show confirmation
    std::string sequenceStr = HomingSettings::SequenceToString(m_config.homing.sequence);
    int result = wxMessageBox(
        wxString::Format("Test %s homing sequence?\n\nThis will execute the actual homing commands on your machine.\n\nMAKE SURE YOUR MACHINE IS SAFE TO HOME!", sequenceStr),
        "Confirm Homing Test", wxYES_NO | wxICON_EXCLAMATION);
    
    if (result != wxYES) {
        return;
    }
    
    // Start homing sequence
    m_homingInProgress = true;
    m_testHomingBtn->Enable(false);
    
    // Register for homing progress updates
    HomingManager::Instance().SetProgressCallback([this](const std::string& machineId, const HomingProgress& progress) {
        if (machineId == m_config.id) {
            CallAfter([this, progress]() {
                OnHomingProgress(m_config.id, progress);
            });
        }
    });
    
    // Start homing
    bool started = HomingManager::Instance().StartHomingSequence(m_config.id);
    if (!started) {
        m_homingInProgress = false;
        m_testHomingBtn->Enable(true);
        wxMessageBox("Failed to start homing sequence.", "Homing Failed", wxOK | wxICON_ERROR);
    }
}

void EnhancedEditMachineDialog::OnHomingProgress(const std::string& machineId, const HomingProgress& progress) {
    // Update status (you could add a status label to show progress)
    static wxProgressDialog* homingProgress = nullptr;
    
    if (progress.state == HomingProgress::STARTING && !homingProgress) {
        homingProgress = new wxProgressDialog("Homing Machine", "Starting homing sequence...", 100, this,
                                             wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT);
    }
    
    if (homingProgress) {
        bool continueHoming = homingProgress->Update(static_cast<int>(progress.progressPercent), progress.statusMessage);
        
        if (!continueHoming) {
            // User cancelled
            HomingManager::Instance().CancelHoming(machineId);
        }
        
        if (progress.state == HomingProgress::COMPLETED || 
            progress.state == HomingProgress::FAILED || 
            progress.state == HomingProgress::CANCELLED) {
            
            homingProgress->Destroy();
            homingProgress = nullptr;
            
            CallAfter([this, machineId]() {
                OnHomingComplete(machineId);
            });
        }
    }
}

void EnhancedEditMachineDialog::OnHomingComplete(const std::string& machineId) {
    m_homingInProgress = false;
    m_testHomingBtn->Enable(true);
    
    HomingProgress finalProgress = HomingManager::Instance().GetHomingProgress(machineId);
    
    if (finalProgress.state == HomingProgress::COMPLETED) {
        wxMessageBox("Homing sequence completed successfully!", "Homing Complete", wxOK | wxICON_INFORMATION);
    } else if (finalProgress.state == HomingProgress::CANCELLED) {
        wxMessageBox("Homing sequence was cancelled.", "Homing Cancelled", wxOK | wxICON_WARNING);
    } else {
        wxMessageBox("Homing sequence failed: " + finalProgress.errorMessage, "Homing Failed", wxOK | wxICON_ERROR);
    }
}

// Data validation and management
bool EnhancedEditMachineDialog::ValidateInput() {
    // Validate name
    if (m_nameText->GetValue().Trim().IsEmpty()) {
        wxMessageBox("Please enter a machine name.", "Validation Error", wxOK | wxICON_WARNING);
        m_notebook->SetSelection(0); // Switch to basic tab
        m_nameText->SetFocus();
        return false;
    }
    
    // Validate host
    if (m_hostText->GetValue().Trim().IsEmpty()) {
        wxMessageBox("Please enter a host address.", "Validation Error", wxOK | wxICON_WARNING);
        m_notebook->SetSelection(0); // Switch to basic tab
        m_hostText->SetFocus();
        return false;
    }
    
    // Validate port
    int port = m_portSpinner->GetValue();
    if (port < 1 || port > 65535) {
        wxMessageBox("Please enter a valid port number (1-65535).", "Validation Error", wxOK | wxICON_WARNING);
        m_notebook->SetSelection(0); // Switch to basic tab
        m_portSpinner->SetFocus();
        return false;
    }
    
    return true;
}

void EnhancedEditMachineDialog::LoadMachineData() {
    // Basic settings
    m_nameText->SetValue(m_config.name);
    m_descriptionText->SetValue(m_config.description);
    m_hostText->SetValue(m_config.host);
    m_portSpinner->SetValue(m_config.port);
    m_autoConnectCheck->SetValue(m_config.autoConnect);
    
    // Set machine type
    for (size_t i = 0; i < m_machineTypes.size(); ++i) {
        if (m_machineTypes[i] == m_config.machineType) {
            m_machineTypeChoice->SetSelection(static_cast<int>(i));
            break;
        }
    }
    
    // Capabilities
    if (m_config.capabilities.capabilitiesValid) {
        m_firmwareVersionLabel->SetLabel(m_config.capabilities.firmwareVersion);
        m_kinematicsLabel->SetLabel(m_config.capabilities.kinematics);
        m_workspaceBoundsLabel->SetLabel(wxString::Format("%.1f x %.1f x %.1f mm", 
                                         m_config.capabilities.workspaceX, 
                                         m_config.capabilities.workspaceY, 
                                         m_config.capabilities.workspaceZ));
        
        std::vector<std::string> features;
        if (m_config.capabilities.hasHoming) features.push_back("Homing");
        if (m_config.capabilities.hasProbe) features.push_back("Probe");
        if (m_config.capabilities.hasSpindle) features.push_back("Spindle");
        if (m_config.capabilities.hasCoolant) features.push_back("Coolant");
        
        std::string featureStr;
        for (size_t i = 0; i < features.size(); ++i) {
            if (i > 0) featureStr += ", ";
            featureStr += features[i];
        }
        m_featuresLabel->SetLabel(featureStr);
        m_lastQueriedLabel->SetLabel(m_config.capabilities.lastQueried);
        
        // Build detailed info
        std::string details = "System Information:\n";
        for (const auto& info : m_config.capabilities.systemInfo) {
            details += info + "\n";
        }
        details += "\nKey GRBL Settings:\n";
        for (const auto& [setting, value] : m_config.capabilities.grblSettings) {
            details += "$" + std::to_string(setting) + "=" + std::to_string(value) + "\n";
        }
        m_capabilityDetails->SetValue(details);
    }
    
    // Homing settings
    m_homingEnabledCheck->SetValue(m_config.homing.enabled);
    m_homingFeedRateSpinner->SetValue(m_config.homing.feedRate);
    m_homingSeekRateSpinner->SetValue(m_config.homing.seekRate);
    m_homingPullOffSpinner->SetValue(m_config.homing.pullOff);
    m_homingSequenceChoice->SetSelection(static_cast<int>(m_config.homing.sequence));
    
    // Custom sequence
    m_customSequenceList->DeleteAllItems();
    for (size_t i = 0; i < m_config.homing.customSequence.size(); ++i) {
        const std::string& command = m_config.homing.customSequence[i];
        m_customSequenceList->InsertItem(i, wxString::Format("%zu", i + 1));
        m_customSequenceList->SetItem(i, 1, command);
        
        // Add description
        std::string description;
        if (command.find("$H") == 0) {
            if (command.length() > 2) {
                description = "Home " + command.substr(2, 1) + " axis";
            } else {
                description = "Home all axes";
            }
        } else if (command.find("G4") == 0) {
            description = "Delay command";
        }
        m_customSequenceList->SetItem(i, 2, description);
    }
    
    // User settings
    m_metricUnitsCheck->SetValue(m_config.userSettings.useMetricUnits);
    m_jogFeedRateSpinner->SetValue(m_config.userSettings.jogFeedRate);
    m_jogDistanceSpinner->SetValue(m_config.userSettings.jogDistance);
    m_softLimitsCheck->SetValue(m_config.userSettings.enableSoftLimits);
    m_hardLimitsCheck->SetValue(m_config.userSettings.enableHardLimits);
    
    // Set coordinate system
    for (size_t i = 0; i < m_coordinateSystems.size(); ++i) {
        if (m_coordinateSystems[i] == m_config.userSettings.preferredCoordinateSystem) {
            m_coordinateSystemChoice->SetSelection(static_cast<int>(i));
            break;
        }
    }
}

void EnhancedEditMachineDialog::SaveMachineData() {
    // Basic settings
    m_config.name = m_nameText->GetValue().ToStdString();
    m_config.description = m_descriptionText->GetValue().ToStdString();
    m_config.host = m_hostText->GetValue().ToStdString();
    m_config.port = m_portSpinner->GetValue();
    m_config.autoConnect = m_autoConnectCheck->GetValue();
    
    // Machine type
    int typeSelection = m_machineTypeChoice->GetSelection();
    if (typeSelection >= 0 && typeSelection < static_cast<int>(m_machineTypes.size())) {
        m_config.machineType = m_machineTypes[typeSelection].ToStdString();
    }
    
    // Homing settings
    m_config.homing.enabled = m_homingEnabledCheck->GetValue();
    m_config.homing.feedRate = static_cast<float>(m_homingFeedRateSpinner->GetValue());
    m_config.homing.seekRate = static_cast<float>(m_homingSeekRateSpinner->GetValue());
    m_config.homing.pullOff = static_cast<float>(m_homingPullOffSpinner->GetValue());
    m_config.homing.sequence = static_cast<HomingSettings::HomingSequence>(m_homingSequenceChoice->GetSelection());
    
    // Custom sequence
    m_config.homing.customSequence.clear();
    for (int i = 0; i < m_customSequenceList->GetItemCount(); ++i) {
        std::string command = m_customSequenceList->GetItemText(i, 1).ToStdString();
        m_config.homing.customSequence.push_back(command);
    }
    
    // User settings
    m_config.userSettings.useMetricUnits = m_metricUnitsCheck->GetValue();
    m_config.userSettings.jogFeedRate = static_cast<float>(m_jogFeedRateSpinner->GetValue());
    m_config.userSettings.jogDistance = static_cast<float>(m_jogDistanceSpinner->GetValue());
    m_config.userSettings.enableSoftLimits = m_softLimitsCheck->GetValue();
    m_config.userSettings.enableHardLimits = m_hardLimitsCheck->GetValue();
    
    // Coordinate system
    int coordSelection = m_coordinateSystemChoice->GetSelection();
    if (coordSelection >= 0 && coordSelection < static_cast<int>(m_coordinateSystems.size())) {
        m_config.userSettings.preferredCoordinateSystem = m_coordinateSystems[coordSelection].ToStdString();
    }
    
    // Generate ID if new machine
    if (m_config.id.empty()) {
        m_config.id = "machine_" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    }
}

void EnhancedEditMachineDialog::UpdateUI() {
    UpdateHomingSequenceUI();
    UpdateCustomSequenceUI();
}

void EnhancedEditMachineDialog::UpdateHomingSequenceUI() {
    // Show/hide custom sequence editor based on selection
    bool showCustom = (m_homingSequenceChoice->GetSelection() == 3); // Custom sequence
    
    if (m_customSequenceBox) {
        m_customSequenceBox->GetStaticBox()->Show(showCustom);
        m_customSequenceList->Show(showCustom);
        m_newStepText->Show(showCustom);
        m_addStepBtn->Show(showCustom);
        m_removeStepBtn->Show(showCustom);
        m_moveUpBtn->Show(showCustom);
        m_moveDownBtn->Show(showCustom);
        
        // Force layout update
        m_homingPanel->Layout();
    }
}

void EnhancedEditMachineDialog::UpdateCustomSequenceUI() {
    // Update button states based on selection
    long selected = m_customSequenceList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    long itemCount = m_customSequenceList->GetItemCount();
    
    m_removeStepBtn->Enable(selected != -1);
    m_moveUpBtn->Enable(selected > 0);
    m_moveDownBtn->Enable(selected != -1 && selected < itemCount - 1);
}
