# FluidNC_gCodeSender - Redesigned Professional Architecture

## 🎯 **Architectural Redesign Overview**

Based on your requirements for a **flexible, responsive, multi-embedded window design**, I've completely redesigned the architecture to be a professional CNC control application with the following key features:

## 🏗️ **Core Architecture Components**

### **1. Enhanced State Management**

#### **StateManager** - Comprehensive Configuration System
```cpp
class StateManager {
    // Multi-machine configurations
    std::vector<MachineConfig> getMachines();
    void addMachine(const MachineConfig& machine);
    
    // Dynamic window layouts
    std::vector<WindowLayout> getWindowLayouts();
    void saveWindowLayout(const WindowLayout& layout);
    
    // Job management with profiles
    JobSettings getCurrentJobSettings();
    void saveJobProfile(const JobSettings& settings);
};
```

**Features:**
- ✅ **Multi-machine support** (Telnet, USB, UART)
- ✅ **Dynamic window layout persistence** (position, size, docking state)
- ✅ **Job profile management** (feeds, speeds, materials, tools)
- ✅ **Hierarchical settings** (General → Job → Machine specific)

#### **Configuration Structure:**
```
config/
├── settings.json      # General application settings
├── machines.json      # Machine configurations
├── job_profiles.json  # Saved job profiles
└── recovery.json      # Auto-saved recovery state
```

### **2. Multi-Protocol Connection Management**

#### **ConnectionManager** - Professional Multi-Machine Client
```cpp
class ConnectionManager {
    // Multi-machine management
    bool addMachine(const MachineConfig& config);
    void setActiveMachine(const std::string& machineId);
    
    // Connection protocols
    std::unique_ptr<IConnection> createConnection(const MachineConfig& config);
    // -> TelnetConnection, USBConnection, UARTConnection
    
    // Real-time status
    MachineStatus getMachineStatus(const std::string& machineId);
    void emergencyStop(const std::string& machineId = ""); // All machines
};
```

**Supported Connection Types:**
- 🌐 **Telnet** - Traditional FluidNC over IP
- 🔌 **USB** - Direct USB connection to CNC controllers  
- 📡 **UART/Serial** - Serial port communication
- 🔄 **Auto-switching** - Seamless switching between machines

### **3. Professional GUI Framework**

#### **MainFrame** - Advanced MDI with Full Customization
```cpp
class MainFrame : public wxFrame {
    // Dynamic panel management
    void ShowPanel(PanelID panelId, bool show = true);
    void TogglePanelVisibility(PanelID panelId);
    void ResetLayout();
    
    // Multi-toolbar support
    wxAuiToolBar* m_mainToolbar;
    wxAuiToolBar* m_machineToolbar; 
    wxAuiToolBar* m_fileToolbar;
    
    // Multi-field status bar
    enum StatusField { STATUS_MAIN, STATUS_MACHINE, STATUS_CONNECTION, STATUS_POSITION };
};
```

**Professional UI Features:**
- 🪟 **Flexible docking** - All panels dockable, floatable, resizable
- 📋 **Multiple toolbars** - Context-sensitive tool groups
- 📊 **Multi-field status bar** - Machine status, connection, coordinates
- 🎛️ **Menu-driven visibility** - Show/hide any panel via menu
- 💾 **Complete layout persistence** - Every window position/size saved

#### **Panel Architecture:**
```
MainFrame (MDI Parent)
├── DROPanel           # Multi-machine coordinates & commands
├── JogPanel           # Machine jogging controls  
├── MachineManagerPanel # Connection & machine switching
├── SVGViewer          # File preview & toolpath display
├── GCodeEditor        # G-code editing & manipulation
├── MacroPanel         # Custom macro management
└── ConsolePanel       # Communication log & diagnostics
```

### **4. Comprehensive Settings System**

#### **SettingsDialog** - Tabbed Professional Configuration
```cpp
class SettingsDialog : public wxDialog {
    // Multi-tab interface
    void CreateGeneralTab();    # Auto-save, units, working directory
    void CreateJobTab();        # Current job settings, profiles
    void CreateMachineTab();    # Machine configurations, limits  
    void CreateConnectionTab(); # Connection settings, timeouts
};
```

**Settings Categories:**

#### 📋 **General Settings Tab**
- Auto-save intervals and recovery
- Default units (mm/inches)
- Working directory management
- UI preferences and tooltips
- Layout restoration settings

#### ⚙️ **Job Settings Tab** 
- Current job parameters (feeds, speeds, depths)
- Material selection (Wood, Aluminum, Steel, etc.)
- Tool management (End mills, V-bits, etc.)  
- Job profile save/load/delete
- Safety settings (safe Z, work Z)

#### 🏭 **Machine Settings Tab**
- Machine configuration list
- Per-machine settings (limits, homing, etc.)
- Machine-specific G-code dialects
- Coordinate system management
- Emergency stop configuration

#### 🔌 **Connection Settings Tab**
- Connection timeouts and retries
- Auto-connect preferences  
- Keep-alive settings
- Communication logging levels
- Multi-machine connection priorities

## 🎛️ **Key Architectural Advantages**

### **1. Multi-Machine Architecture**
```cpp
// Seamless machine switching
connectionManager->setActiveMachine("CNC_Router_01");
connectionManager->sendCommandToActive("G0 X10 Y10");

// Simultaneous monitoring
auto statuses = connectionManager->getAllStatuses();
for (const auto& status : statuses) {
    droPanel->UpdateMachineStatus(status.machineId, status);
}
```

### **2. Dynamic Layout System**
```cpp
// Complete layout flexibility
mainFrame->ShowPanel(PANEL_DRO, true);
mainFrame->TogglePanelVisibility(PANEL_GCODE_EDITOR);
mainFrame->SaveCurrentLayout();  // Automatically persisted

// Window position restoration
WindowLayout layout = stateManager.getWindowLayout("DROPanel");
// Position: {x: 100, y: 200, width: 400, height: 300, docked: true}
```

### **3. Professional Settings Management** 
```cpp
// Hierarchical configuration
stateManager.setValue("general/units", "mm");
stateManager.setValue("machines/router01/maxFeedRate", 5000.0f);
stateManager.setValue("jobs/current/material", "Aluminum");

// Job profile system
JobSettings profile = stateManager.getCurrentJobSettings();
profile.feedRate = 1200.0f;
profile.material = "Hardwood";
stateManager.saveJobProfile(profile);
```

## 🔧 **Implementation Plan**

### **Phase 2A: Core Foundation Enhancement** 🚧
1. **Complete StateManager redesign** with multi-machine/job support
2. **Implement ConnectionManager** with multi-protocol support  
3. **Create base IConnection interface** and TelnetConnection implementation

### **Phase 2B: Professional GUI** 🚧
1. **Implement MainFrame** with AUI manager, toolbars, status bar
2. **Create SettingsDialog** with all four tabs
3. **Build DROPanel** with multi-machine coordinate display
4. **Add JogPanel** with machine-specific jogging

### **Phase 2C: Advanced Features** ❌
1. **USB/UART connection implementations**
2. **SVGViewer with toolpath preview**
3. **GCodeEditor with drag-drop manipulation**
4. **MacroPanel with custom scripting**

## 📂 **Updated Project Structure**

```
FluidNC_gCodeSender/
├── src/
│   ├── core/
│   │   ├── StateManager.{h,cpp}      # ✅ Enhanced multi-config
│   │   ├── ConnectionManager.{h,cpp}  # 🚧 Multi-protocol manager
│   │   ├── TelnetConnection.{h,cpp}   # 🚧 Telnet implementation
│   │   ├── USBConnection.{h,cpp}      # ❌ USB implementation
│   │   └── UARTConnection.{h,cpp}     # ❌ UART implementation
│   │
│   └── gui/
│       ├── MainFrame.{h,cpp}         # 🚧 Professional MDI
│       ├── SettingsDialog.{h,cpp}    # 🚧 Comprehensive settings
│       ├── DROPanel.{h,cpp}          # 🚧 Multi-machine DRO
│       ├── JogPanel.{h,cpp}          # 🚧 Machine control
│       ├── MachineManagerPanel.{h,cpp} # ❌ Connection management
│       ├── SVGViewer.{h,cpp}         # ❌ File preview
│       ├── GCodeEditor.{h,cpp}       # ❌ Code editing
│       ├── MacroPanel.{h,cpp}        # ❌ Macro management
│       └── ConsolePanel.{h,cpp}      # ❌ Communication log
│
├── config/                           # Auto-created configuration
│   ├── settings.json
│   ├── machines.json
│   ├── job_profiles.json
│   └── recovery.json
│
└── resources/                        # Application resources
    ├── icons/
    ├── toolbars/
    └── help/
```

## 🎯 **Immediate Next Steps**

### **Priority 1: Enhanced State Management**
1. Implement the redesigned StateManager with:
   - MachineConfig management
   - WindowLayout persistence  
   - JobSettings profiles
   - Multi-file configuration system

### **Priority 2: Multi-Machine Connection System**
1. Create ConnectionManager base architecture
2. Implement IConnection interface
3. Build TelnetConnection as first implementation
4. Add USB/UART stubs for future expansion

### **Priority 3: Professional Main Frame**
1. Implement MainFrame with:
   - AUI docking manager
   - Multiple toolbars
   - Multi-field status bar
   - Dynamic panel management
   - Complete layout persistence

### **Priority 4: Comprehensive Settings**
1. Build SettingsDialog with all tabs:
   - General application preferences
   - Current job parameter management
   - Machine configuration interface
   - Connection settings control

This redesigned architecture addresses all your requirements:
- ✅ **Flexible window management** - Full AUI docking system
- ✅ **Dynamic layout persistence** - Every window position/size saved
- ✅ **Multi-machine support** - Configurable connections (Telnet/USB/UART)
- ✅ **Professional settings** - Tabbed dialog with all configuration options  
- ✅ **Responsive design** - Modern C++ with efficient threading

The result will be a **professional-grade CNC control application** that rivals commercial software in functionality and flexibility.
