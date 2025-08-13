/**
 * gui/EnhancedEditMachineDialog_Part2.cpp
 * Continuation of Enhanced Edit Machine Dialog implementation
 * This file should be merged with the main implementation file
 */

// This is a continuation of the EnhancedEditMachineDialog.cpp file

void EnhancedEditMachineDialog::CreateHomingTab(wxNotebook* notebook) {
    m_homingPanel = new wxPanel(notebook);
    notebook->AddPage(m_homingPanel, "Homing Settings");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Homing enable/disable
    m_homingEnabledCheck = new wxCheckBox(m_homingPanel, wxID_ANY, "Enable Homing");
    m_homingEnabledCheck->SetToolTip("Enable or disable homing functionality for this machine");
    mainSizer->Add(m_homingEnabledCheck, 0, wxALL, 10);
    
    // Homing parameters section
    wxStaticBoxSizer* parametersBox = new wxStaticBoxSizer(wxVERTICAL, m_homingPanel, "Homing Parameters");
    
    wxFlexGridSizer* parametersGrid = new wxFlexGridSizer(3, 2, 5, 10);
    parametersGrid->AddGrowableCol(1, 1);
    
    // Feed Rate
    parametersGrid->Add(new wxStaticText(m_homingPanel, wxID_ANY, "Feed Rate (mm/min):"), 0, wxALIGN_CENTER_VERTICAL);
    m_homingFeedRateSpinner = new wxSpinCtrlDouble(m_homingPanel, wxID_ANY);
    m_homingFeedRateSpinner->SetRange(1.0, 10000.0);
    m_homingFeedRateSpinner->SetValue(500.0);
    m_homingFeedRateSpinner->SetIncrement(10.0);
    m_homingFeedRateSpinner->SetToolTip("Feed rate used during homing operations");
    parametersGrid->Add(m_homingFeedRateSpinner, 1, wxEXPAND);
    
    // Seek Rate
    parametersGrid->Add(new wxStaticText(m_homingPanel, wxID_ANY, "Seek Rate (mm/min):"), 0, wxALIGN_CENTER_VERTICAL);
    m_homingSeekRateSpinner = new wxSpinCtrlDouble(m_homingPanel, wxID_ANY);
    m_homingSeekRateSpinner->SetRange(1.0, 10000.0);
    m_homingSeekRateSpinner->SetValue(2500.0);
    m_homingSeekRateSpinner->SetIncrement(50.0);
    m_homingSeekRateSpinner->SetToolTip("Initial rapid seek rate to find limit switches");
    parametersGrid->Add(m_homingSeekRateSpinner, 1, wxEXPAND);
    
    // Pull-off Distance
    parametersGrid->Add(new wxStaticText(m_homingPanel, wxID_ANY, "Pull-off Distance (mm):"), 0, wxALIGN_CENTER_VERTICAL);
    m_homingPullOffSpinner = new wxSpinCtrlDouble(m_homingPanel, wxID_ANY);
    m_homingPullOffSpinner->SetRange(0.1, 10.0);
    m_homingPullOffSpinner->SetValue(1.0);
    m_homingPullOffSpinner->SetIncrement(0.1);
    m_homingPullOffSpinner->SetDigits(1);
    m_homingPullOffSpinner->SetToolTip("Distance to back off from limit switches after homing");
    parametersGrid->Add(m_homingPullOffSpinner, 1, wxEXPAND);
    
    parametersBox->Add(parametersGrid, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(parametersBox, 0, wxALL | wxEXPAND, 5);
    
    // Homing sequence section
    wxStaticBoxSizer* sequenceBox = new wxStaticBoxSizer(wxVERTICAL, m_homingPanel, "Homing Sequence");
    
    wxBoxSizer* sequenceChoiceSizer = new wxBoxSizer(wxHORIZONTAL);
    sequenceChoiceSizer->Add(new wxStaticText(m_homingPanel, wxID_ANY, "Sequence Type:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    m_homingSequenceChoice = new wxChoice(m_homingPanel, ID_HOMING_SEQUENCE_CHOICE);
    m_homingSequenceChoice->Append("Simultaneous (Cartesian)");
    m_homingSequenceChoice->Append("Sequential Z->X->Y (CoreXY)");
    m_homingSequenceChoice->Append("Sequential Z->Y->X (Alternative)");
    m_homingSequenceChoice->Append("Custom Sequence");
    m_homingSequenceChoice->SetSelection(0);
    m_homingSequenceChoice->SetToolTip("Choose homing sequence based on machine kinematics");
    
    sequenceChoiceSizer->Add(m_homingSequenceChoice, 1, wxEXPAND);
    sequenceBox->Add(sequenceChoiceSizer, 0, wxALL | wxEXPAND, 5);
    
    // Custom sequence editor
    CreateCustomSequenceEditor(m_homingPanel);
    
    mainSizer->Add(sequenceBox, 1, wxALL | wxEXPAND, 5);
    
    // Test homing button
    m_testHomingBtn = new wxButton(m_homingPanel, ID_TEST_HOMING, "Test Homing Sequence");
    m_testHomingBtn->SetToolTip("Test the configured homing sequence (requires connected machine)");
    mainSizer->Add(m_testHomingBtn, 0, wxALL | wxCENTER, 10);
    
    m_homingPanel->SetSizer(mainSizer);
}

void EnhancedEditMachineDialog::CreateCustomSequenceEditor(wxPanel* parent) {
    m_customSequenceBox = new wxStaticBoxSizer(wxVERTICAL, parent, "Custom Sequence Editor");
    
    // List of custom commands
    m_customSequenceList = new wxListCtrl(parent, ID_CUSTOM_SEQUENCE_LIST, wxDefaultPosition, wxSize(-1, 120), 
                                           wxLC_REPORT | wxLC_SINGLE_SEL);
    m_customSequenceList->AppendColumn("Step", wxLIST_FORMAT_LEFT, 50);
    m_customSequenceList->AppendColumn("Command", wxLIST_FORMAT_LEFT, 200);
    m_customSequenceList->AppendColumn("Description", wxLIST_FORMAT_LEFT, 300);
    m_customSequenceList->SetToolTip("Custom homing sequence commands. Use $HZ, $HX, $HY for individual axes, G4 P500 for delays");
    
    m_customSequenceBox->Add(m_customSequenceList, 1, wxALL | wxEXPAND, 5);
    
    // Command input and buttons
    wxBoxSizer* commandInputSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_newStepText = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxSize(150, -1));
    m_newStepText->SetToolTip("Enter new command ($HX, $HY, $HZ, G4 P500, etc.)");
    commandInputSizer->Add(m_newStepText, 1, wxRIGHT, 5);
    
    m_addStepBtn = new wxButton(parent, ID_ADD_HOMING_STEP, "Add");
    m_removeStepBtn = new wxButton(parent, ID_REMOVE_HOMING_STEP, "Remove");
    m_moveUpBtn = new wxButton(parent, ID_MOVE_STEP_UP, "Up");
    m_moveDownBtn = new wxButton(parent, ID_MOVE_STEP_DOWN, "Down");
    
    commandInputSizer->Add(m_addStepBtn, 0, wxRIGHT, 3);
    commandInputSizer->Add(m_removeStepBtn, 0, wxRIGHT, 3);
    commandInputSizer->Add(m_moveUpBtn, 0, wxRIGHT, 3);
    commandInputSizer->Add(m_moveDownBtn, 0);
    
    m_customSequenceBox->Add(commandInputSizer, 0, wxALL | wxEXPAND, 5);
    
    // Add the box to the parent - this will be done by the calling method
    // The caller should: mainSizer->Add(m_customSequenceBox, 1, wxALL | wxEXPAND, 5);
}

void EnhancedEditMachineDialog::CreateUserSettingsTab(wxNotebook* notebook) {
    m_userSettingsPanel = new wxPanel(notebook);
    notebook->AddPage(m_userSettingsPanel, "User Preferences");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Units and display preferences
    wxStaticBoxSizer* displayBox = new wxStaticBoxSizer(wxVERTICAL, m_userSettingsPanel, "Display Preferences");
    
    m_metricUnitsCheck = new wxCheckBox(m_userSettingsPanel, wxID_ANY, "Use Metric Units (mm)");
    m_metricUnitsCheck->SetValue(true);
    m_metricUnitsCheck->SetToolTip("Use metric units (mm) instead of imperial (inches)");
    displayBox->Add(m_metricUnitsCheck, 0, wxALL, 5);
    
    mainSizer->Add(displayBox, 0, wxALL | wxEXPAND, 5);
    
    // Jogging preferences
    wxStaticBoxSizer* jogBox = new wxStaticBoxSizer(wxVERTICAL, m_userSettingsPanel, "Jogging Preferences");
    
    wxFlexGridSizer* jogGrid = new wxFlexGridSizer(2, 2, 5, 10);
    jogGrid->AddGrowableCol(1, 1);
    
    // Default jog feed rate
    jogGrid->Add(new wxStaticText(m_userSettingsPanel, wxID_ANY, "Jog Feed Rate (mm/min):"), 0, wxALIGN_CENTER_VERTICAL);
    m_jogFeedRateSpinner = new wxSpinCtrlDouble(m_userSettingsPanel, wxID_ANY);
    m_jogFeedRateSpinner->SetRange(1.0, 10000.0);
    m_jogFeedRateSpinner->SetValue(1000.0);
    m_jogFeedRateSpinner->SetIncrement(100.0);
    m_jogFeedRateSpinner->SetToolTip("Default feed rate for jogging operations");
    jogGrid->Add(m_jogFeedRateSpinner, 1, wxEXPAND);
    
    // Default jog distance
    jogGrid->Add(new wxStaticText(m_userSettingsPanel, wxID_ANY, "Jog Distance (mm):"), 0, wxALIGN_CENTER_VERTICAL);
    m_jogDistanceSpinner = new wxSpinCtrlDouble(m_userSettingsPanel, wxID_ANY);
    m_jogDistanceSpinner->SetRange(0.01, 100.0);
    m_jogDistanceSpinner->SetValue(1.0);
    m_jogDistanceSpinner->SetIncrement(0.1);
    m_jogDistanceSpinner->SetDigits(2);
    m_jogDistanceSpinner->SetToolTip("Default distance for jogging operations");
    jogGrid->Add(m_jogDistanceSpinner, 1, wxEXPAND);
    
    jogBox->Add(jogGrid, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(jogBox, 0, wxALL | wxEXPAND, 5);
    
    // Safety preferences
    wxStaticBoxSizer* safetyBox = new wxStaticBoxSizer(wxVERTICAL, m_userSettingsPanel, "Safety Settings");
    
    m_softLimitsCheck = new wxCheckBox(m_userSettingsPanel, wxID_ANY, "Enable Soft Limits");
    m_softLimitsCheck->SetValue(true);
    m_softLimitsCheck->SetToolTip("Enable software-enforced workspace limits");
    safetyBox->Add(m_softLimitsCheck, 0, wxALL, 5);
    
    m_hardLimitsCheck = new wxCheckBox(m_userSettingsPanel, wxID_ANY, "Enable Hard Limits");
    m_hardLimitsCheck->SetValue(true);
    m_hardLimitsCheck->SetToolTip("Enable hardware limit switch monitoring");
    safetyBox->Add(m_hardLimitsCheck, 0, wxALL, 5);
    
    mainSizer->Add(safetyBox, 0, wxALL | wxEXPAND, 5);
    
    // Coordinate system preference
    wxStaticBoxSizer* coordBox = new wxStaticBoxSizer(wxVERTICAL, m_userSettingsPanel, "Coordinate System");
    
    wxBoxSizer* coordSizer = new wxBoxSizer(wxHORIZONTAL);
    coordSizer->Add(new wxStaticText(m_userSettingsPanel, wxID_ANY, "Preferred System:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    m_coordinateSystemChoice = new wxChoice(m_userSettingsPanel, wxID_ANY);
    for (const auto& cs : m_coordinateSystems) {
        m_coordinateSystemChoice->Append(cs);
    }
    m_coordinateSystemChoice->SetSelection(0); // Default to G54
    m_coordinateSystemChoice->SetToolTip("Default coordinate system to use");
    coordSizer->Add(m_coordinateSystemChoice, 1, wxEXPAND);
    
    coordBox->Add(coordSizer, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(coordBox, 0, wxALL | wxEXPAND, 5);
    
    // Add spacer
    mainSizer->AddStretchSpacer(1);
    
    m_userSettingsPanel->SetSizer(mainSizer);
}

// Homing event handlers
void EnhancedEditMachineDialog::OnHomingSequenceChanged(wxCommandEvent& event) {
    UpdateHomingSequenceUI();
}

void EnhancedEditMachineDialog::OnAddHomingStep(wxCommandEvent& event) {
    wxString command = m_newStepText->GetValue().Trim();
    if (command.IsEmpty()) {
        return;
    }
    
    // Basic validation
    if (!command.StartsWith("$H") && !command.StartsWith("G4")) {
        wxMessageBox("Invalid command. Use $HX, $HY, $HZ for axis homing, or G4 P<ms> for delays.", 
                     "Invalid Command", wxOK | wxICON_WARNING);
        return;
    }
    
    // Add to list
    long index = m_customSequenceList->GetItemCount();
    m_customSequenceList->InsertItem(index, wxString::Format("%ld", index + 1));
    m_customSequenceList->SetItem(index, 1, command);
    
    // Add description
    wxString description;
    if (command.StartsWith("$H")) {
        wxString axis = command.Mid(2, 1);
        description = "Home " + axis + " axis";
    } else if (command.StartsWith("G4")) {
        description = "Delay command";
    }
    m_customSequenceList->SetItem(index, 2, description);
    
    // Clear input
    m_newStepText->Clear();
    
    UpdateCustomSequenceUI();
}

void EnhancedEditMachineDialog::OnRemoveHomingStep(wxCommandEvent& event) {
    long selected = m_customSequenceList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (selected != -1) {
        m_customSequenceList->DeleteItem(selected);
        
        // Renumber remaining items
        for (long i = 0; i < m_customSequenceList->GetItemCount(); ++i) {
            m_customSequenceList->SetItem(i, 0, wxString::Format("%ld", i + 1));
        }
        
        UpdateCustomSequenceUI();
    }
}

void EnhancedEditMachineDialog::OnMoveHomingStepUp(wxCommandEvent& event) {
    long selected = m_customSequenceList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (selected > 0) {
        // Get both items
        wxString cmd1 = m_customSequenceList->GetItemText(selected - 1, 1);
        wxString desc1 = m_customSequenceList->GetItemText(selected - 1, 2);
        wxString cmd2 = m_customSequenceList->GetItemText(selected, 1);
        wxString desc2 = m_customSequenceList->GetItemText(selected, 2);
        
        // Swap them
        m_customSequenceList->SetItem(selected - 1, 1, cmd2);
        m_customSequenceList->SetItem(selected - 1, 2, desc2);
        m_customSequenceList->SetItem(selected, 1, cmd1);
        m_customSequenceList->SetItem(selected, 2, desc1);
        
        // Update selection
        m_customSequenceList->SetItemState(selected, 0, wxLIST_STATE_SELECTED);
        m_customSequenceList->SetItemState(selected - 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        
        UpdateCustomSequenceUI();
    }
}

void EnhancedEditMachineDialog::OnMoveHomingStepDown(wxCommandEvent& event) {
    long selected = m_customSequenceList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (selected != -1 && selected < m_customSequenceList->GetItemCount() - 1) {
        // Get both items
        wxString cmd1 = m_customSequenceList->GetItemText(selected, 1);
        wxString desc1 = m_customSequenceList->GetItemText(selected, 2);
        wxString cmd2 = m_customSequenceList->GetItemText(selected + 1, 1);
        wxString desc2 = m_customSequenceList->GetItemText(selected + 1, 2);
        
        // Swap them
        m_customSequenceList->SetItem(selected, 1, cmd2);
        m_customSequenceList->SetItem(selected, 2, desc2);
        m_customSequenceList->SetItem(selected + 1, 1, cmd1);
        m_customSequenceList->SetItem(selected + 1, 2, desc1);
        
        // Update selection
        m_customSequenceList->SetItemState(selected, 0, wxLIST_STATE_SELECTED);
        m_customSequenceList->SetItemState(selected + 1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        
        UpdateCustomSequenceUI();
    }
}

void EnhancedEditMachineDialog::OnHomingStepSelected(wxListEvent& event) {
    UpdateCustomSequenceUI();
}

void EnhancedEditMachineDialog::OnHomingStepEdit(wxListEvent& event) {
    long selected = event.GetIndex();
    if (selected != -1) {
        wxString currentCommand = m_customSequenceList->GetItemText(selected, 1);
        wxString newCommand = wxGetTextFromUser("Edit command:", "Edit Homing Step", currentCommand, this);
        
        if (!newCommand.IsEmpty() && newCommand != currentCommand) {
            // Basic validation
            if (!newCommand.StartsWith("$H") && !newCommand.StartsWith("G4")) {
                wxMessageBox("Invalid command. Use $HX, $HY, $HZ for axis homing, or G4 P<ms> for delays.", 
                             "Invalid Command", wxOK | wxICON_WARNING);
                return;
            }
            
            m_customSequenceList->SetItem(selected, 1, newCommand);
            
            // Update description
            wxString description;
            if (newCommand.StartsWith("$H")) {
                wxString axis = newCommand.Mid(2, 1);
                description = "Home " + axis + " axis";
            } else if (newCommand.StartsWith("G4")) {
                description = "Delay command";
            }
            m_customSequenceList->SetItem(selected, 2, description);
            
            UpdateCustomSequenceUI();
        }
    }
}

void EnhancedEditMachineDialog::OnTestHoming(wxCommandEvent& event) {
    TestHomingSequence();
}
