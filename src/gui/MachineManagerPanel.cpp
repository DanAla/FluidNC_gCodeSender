/**
 * gui/MachineManagerPanel.cpp
 * Machine Manager Panel implementation.
 *
 * This is the main entry point for the MachineManagerPanel class.
 * The implementation is split into several files included below.
 */

#include "MachineManagerPanel.h"
#include "core/SimpleLogger.h"

// The constructor initializes the panel by calling methods that are defined
// in the included files below.
MachineManagerPanel::MachineManagerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), m_splitter(nullptr)
{
    // These methods are defined in the included files
    CreateControls();
    LoadMachineConfigs();
    PopulateMachineList();
    LOG_INFO("MachineManagerPanel initialized.");
}

// Include the separated implementation files
#include "MachineManagerPanel_UI.cpp"
#include "MachineManagerPanel_Events.cpp"
#include "MachineManagerPanel_Data.cpp"
#include "MachineManagerPanel_Helpers.cpp"


