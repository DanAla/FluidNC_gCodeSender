/**
 * gui/MachineManagerPanel_Data.cpp
 *
 * Data handling implementation for the MachineManagerPanel class.
 */

#include "MachineManagerPanel.h"
#include "core/SimpleLogger.h"
#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <fstream>
#include <json.hpp>


void MachineManagerPanel::LoadMachineConfigs()
{
    m_machines.clear();
    
    wxString settingsPath = GetSettingsPath();
    wxString machinesFile = settingsPath + wxFileName::GetPathSeparator() + "machines.json";
    
    // Check if machines.json exists
    if (!wxFileName::FileExists(machinesFile)) {
        // Create empty machines file
        CreateEmptyMachinesFile(machinesFile);
        return; // Start with empty machine list
    }
    
    try {
        // Read the JSON file
        std::ifstream file(machinesFile.ToStdString());
        if (!file.is_open()) {
            LOG_ERROR("Could not open machines.json file for reading");
            return;
        }
        
        nlohmann::json j;
        file >> j;
        file.close();
        
        // Parse machines from JSON
        if (j.contains("machines") && j["machines"].is_array()) {
            for (const auto& machineJson : j["machines"]) {
                MachineConfig machine;
                
                machine.id = machineJson.value("id", "");
                machine.name = machineJson.value("name", "");
                machine.description = machineJson.value("description", "");
                machine.host = machineJson.value("host", "");
                machine.port = machineJson.value("port", 23);
                machine.machineType = machineJson.value("machineType", "FluidNC");
                machine.connected = false; // Always start disconnected
                machine.lastConnected = machineJson.value("lastConnected", "Never");
                machine.autoConnect = machineJson.value("autoConnect", false);
                
                m_machines.push_back(machine);
            }
        }
        
        std::string logMsg = "Loaded " + std::to_string(m_machines.size()) + " machine configurations from " + machinesFile.ToStdString();
        LOG_INFO(logMsg);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading machine configurations: " + std::string(e.what()));
        // Continue with empty machine list on error
    }
}

void MachineManagerPanel::SaveMachineConfigs()
{
    wxString settingsPath = GetSettingsPath();
    if (settingsPath.IsEmpty()) {
        LOG_ERROR("Could not determine settings path");
        return;
    }
    
    wxString machinesFile = settingsPath + wxFileName::GetPathSeparator() + "machines.json";
    
    try {
        nlohmann::json j;
        j["machines"] = nlohmann::json::array();
        j["version"] = "1.0";
        j["lastModified"] = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S").ToStdString();
        
        // Convert machines to JSON
        for (const auto& machine : m_machines) {
            nlohmann::json machineJson;
            machineJson["id"] = machine.id;
            machineJson["name"] = machine.name;
            machineJson["description"] = machine.description;
            machineJson["host"] = machine.host;
            machineJson["port"] = machine.port;
            machineJson["machineType"] = machine.machineType;
            machineJson["lastConnected"] = machine.lastConnected;
            machineJson["autoConnect"] = machine.autoConnect;
            
            j["machines"].push_back(machineJson);
        }
        
        // Write to file
        std::ofstream file(machinesFile.ToStdString());
        if (file.is_open()) {
            file << j.dump(4); // Pretty print with 4-space indentation
            file.close();
            std::string saveMsg = "Saved " + std::to_string(m_machines.size()) + " machine configurations to " + machinesFile.ToStdString();
            LOG_INFO(saveMsg);
        } else {
            LOG_ERROR("Could not open machines.json file for writing: " + machinesFile.ToStdString());
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error saving machine configurations: " + std::string(e.what()));
    }
}

wxString MachineManagerPanel::GetSettingsPath()
{
    // Get user's app data directory
    wxString appDataDir = wxStandardPaths::Get().GetUserDataDir();
    
    // Create our settings subdirectory
    wxString settingsDir = appDataDir + wxFileName::GetPathSeparator() + "settings";
    
    // Ensure the settings directory exists
    if (!wxDir::Exists(settingsDir)) {
        if (!wxFileName::Mkdir(settingsDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
            LOG_ERROR("Could not create settings directory: " + settingsDir.ToStdString());
            return wxEmptyString;
        }
        LOG_INFO("Created settings directory: " + settingsDir.ToStdString());
    }
    
    return settingsDir;
}

void MachineManagerPanel::CreateEmptyMachinesFile(const wxString& filePath)
{
    try {
        nlohmann::json j;
        j["machines"] = nlohmann::json::array();
        j["version"] = "1.0";
        j["created"] = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S").ToStdString();
        
        std::ofstream file(filePath.ToStdString());
        if (file.is_open()) {
            file << j.dump(4); // Pretty print with 4-space indentation
            file.close();
            LOG_INFO("Created empty machines.json file: " + filePath.ToStdString());
        } else {
            LOG_ERROR("Could not create machines.json file: " + filePath.ToStdString());
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error creating machines.json file: " + std::string(e.what()));
    }
}
