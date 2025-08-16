/**
 * gui/MachineManagerPanel_Helpers.cpp
 *
 * Helper methods implementation for the MachineManagerPanel class.
 */

#include "MachineManagerPanel.h"
#include "core/SimpleLogger.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif


void MachineManagerPanel::PopulateMachineList()
{
    m_machineList->DeleteAllItems();
    
    for (size_t i = 0; i < m_machines.size(); ++i) {
        const auto& machine = m_machines[i];
        
        long itemIndex = m_machineList->InsertItem(i, machine.name);
        m_machineList->SetItem(itemIndex, 1, machine.host);
        m_machineList->SetItem(itemIndex, 2, std::to_string(machine.port));
        m_machineList->SetItem(itemIndex, 3, machine.connected ? "Connected" : "Disconnected");
        
        // Store machine ID in item data
        m_machineList->SetItemData(itemIndex, i);
        
        // Color coding - use dark green for better visibility
        if (machine.connected) {
            m_machineList->SetItemTextColour(itemIndex, wxColour(0, 128, 0)); // Dark green
        } else {
            m_machineList->SetItemTextColour(itemIndex, *wxBLACK);
        }
    }
}

void MachineManagerPanel::RefreshMachineList()
{
    PopulateMachineList();
}

void MachineManagerPanel::SelectMachine(const std::string& machineId)
{
    m_selectedMachine = machineId;
    UpdateMachineDetails();
}

void MachineManagerPanel::UpdateConnectionStatus(const std::string& machineId, bool connected)
{
    // Find and update machine status
    for (auto& machine : m_machines) {
        if (machine.id == machineId) {
            machine.connected = connected;
            if (connected) {
                machine.lastConnected = "Connected";
            }
            break;
        }
    }
    RefreshMachineList();
    if (m_selectedMachine == machineId) {
        UpdateMachineDetails();
    }
}

void MachineManagerPanel::UpdateMachineDetails()
{
    // Find selected machine
    const MachineConfig* selectedMachine = nullptr;
    for (const auto& machine : m_machines) {
        if (machine.id == m_selectedMachine) {
            selectedMachine = &machine;
            break;
        }
    }
    
    if (selectedMachine) {
        m_nameLabel->SetLabel(selectedMachine->name);
        
        // Set description with proper handling for empty descriptions
        wxString description = selectedMachine->description.empty() ? "No description" : selectedMachine->description;
        m_descriptionLabel->SetLabel(description);
        
        // Initial wrap
        m_detailsPanel->Layout();
        m_descriptionLabel->Wrap(m_descriptionSizer->GetSize().GetWidth());
        
        m_hostLabel->SetLabel(selectedMachine->host);
        m_portLabel->SetLabel(std::to_string(selectedMachine->port));
        m_machineTypeLabel->SetLabel(selectedMachine->machineType);
        m_statusLabel->SetLabel(selectedMachine->connected ? "Connected" : "Disconnected");
        m_lastConnectedLabel->SetLabel(selectedMachine->lastConnected);
        
        // Update button states
        m_editBtn->Enable(true);
        m_removeBtn->Enable(true);
        m_connectBtn->Enable(!selectedMachine->connected);
        m_disconnectBtn->Enable(selectedMachine->connected);
        m_testBtn->Enable(true);
        
        // Color status label - use dark green for better visibility
        if (selectedMachine->connected) {
            m_statusLabel->SetForegroundColour(wxColour(0, 128, 0)); // Dark green
        } else {
            m_statusLabel->SetForegroundColour(*wxRED);
        }
    } else {
        // Clear details
        m_nameLabel->SetLabel("-");
        m_descriptionLabel->SetLabel("-");
        m_hostLabel->SetLabel("-");
        m_portLabel->SetLabel("-");
        m_machineTypeLabel->SetLabel("-");
        m_statusLabel->SetLabel("-");
        m_lastConnectedLabel->SetLabel("-");
        
        // Disable buttons
        m_editBtn->Enable(false);
        m_removeBtn->Enable(false);
        m_connectBtn->Enable(false);
        m_disconnectBtn->Enable(false);
        m_testBtn->Enable(false);
        
        m_statusLabel->SetForegroundColour(*wxBLACK);
    }
    
    m_detailsPanel->Layout();
}


bool MachineManagerPanel::TestTelnetConnection(const std::string& host, int port)
{
    // Use NetworkManager for consistent connection handling
    NetworkManager& networkManager = NetworkManager::getInstance();
    if (!networkManager.isInitialized()) {
        if (!networkManager.initialize()) {
            LOG_ERROR("Failed to initialize NetworkManager");
            return false;
        }
    }
    
    return networkManager.testTcpPort(host, port);
}
