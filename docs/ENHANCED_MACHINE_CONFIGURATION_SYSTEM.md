# Enhanced Machine Configuration System

## Vision: Comprehensive Machine-Aware Application

The application should be **fully machine-aware**, with every panel and feature automatically adapting to the specific capabilities and limits of the connected machine. No more hardcoded values - everything should come from the actual machine configuration.

## Core Architecture

### 1. Enhanced Edit Machine Dialog
**Replaces the basic "Add Machine" dialog with a comprehensive multi-tab interface:**

#### **Tab 1: Basic Info**
- Name, Description, Connection settings
- Host, Port, Protocol, Auto-connect

#### **Tab 2: Motion & Workspace** 
- **Travel Limits**: X/Y/Z max travel from FluidNC `$130`, `$131`, `$132`
- **Feed Rates**: Maximum rates from `$110`, `$111`, `$112`  
- **Acceleration**: Settings from `$120`, `$121`, `$122`
- **Steps/mm**: Motor configuration from `$100`, `$101`, `$102`

#### **Tab 3: Homing & Safety**
- **Homing Enable**: From `$23`
- **Homing Speeds**: Feed rate `$24`, seek rate `$25`  
- **Safety Settings**: Debounce `$26`, pull-off `$27`
- **Direction Mask**: Which axes home positive

#### **Tab 4: Spindle & Laser**
- **RPM Limits**: Min/max from `$30`, `$31`
- **Laser Mode**: Enable/disable from `$32`
- **PWM Settings**: Frequency and control parameters

#### **Tab 5: Probe & Touch**
- **Probe Enable**: From probe pin configuration
- **Probe Settings**: Feed rates, distances, invert flags
- **Touch Plate**: Thickness and usage settings

#### **Tab 6: Firmware Info**
- **Version**: From `$I` command response
- **Build Info**: Compile date, features, board type
- **Capabilities**: Detected features and extensions

#### **Tab 7: Advanced Settings**
- **Complete GRBL Grid**: All `$nnn` settings in editable table
- **Coordinate Systems**: G54-G59 offsets from `$#` command
- **Real-time Overrides**: Feed, spindle, rapid override limits

### 2. Auto-Discovery Button
**"Query Machine" button that:**
- Connects to the machine temporarily
- Sends discovery commands: `$$`, `$I`, `$G`, `$#`
- Parses ALL responses automatically
- Populates ALL dialog fields with real values
- Shows progress bar during discovery
- Validates discovered settings for conflicts
- Saves complete machine profile

### 3. MachineConfigManager (Singleton)
**Centralized access point for ALL machine data:**

```cpp
// Quick access methods
float GetWorkspaceX(machineId);        // For visualization bounds
float GetMaxFeedRate(machineId);       // For jog panel limits  
bool HasHoming(machineId);             // For UI button states
float GetMaxSpindleRPM(machineId);     // For spindle controls
std::string GetFirmwareVersion(machineId); // For compatibility checks

// Active machine shortcuts
std::string GetActiveMachineId();
CompleteMachineConfig GetActiveMachine();

// Convenience macros
ACTIVE_MACHINE_WORKSPACE_X()   // Quick workspace access
ACTIVE_MACHINE_MAX_FEED()      // Quick feed rate access
```

## FluidNC Command Discovery Protocol

### Discovery Commands Sequence
```
1. $$     - Get ALL machine settings (130+ parameters)
2. $I     - Get build info and version
3. $G     - Get current parser state  
4. $#     - Get coordinate system offsets
5. $N     - Get startup blocks
6. ?      - Get real-time status for validation
```

### Key Settings Extraction
```cpp
// Travel limits (workspace bounds)
$130 = 300.000   // X-axis maximum travel → motion.maxTravelX = 300.0f
$131 = 200.000   // Y-axis maximum travel → motion.maxTravelY = 200.0f  
$132 = 100.000   // Z-axis maximum travel → motion.maxTravelZ = 100.0f

// Feed rate limits
$110 = 3000.000  // X-axis maximum rate → motion.maxRateX = 3000.0f
$111 = 3000.000  // Y-axis maximum rate → motion.maxRateY = 3000.0f
$112 = 500.000   // Z-axis maximum rate → motion.maxRateZ = 500.0f

// Homing configuration  
$23 = 1          // Homing enable → homing.enabled = true
$24 = 500.000    // Homing feed rate → homing.feedRate = 500.0f
$25 = 2500.000   // Homing seek rate → homing.seekRate = 2500.0f

// Spindle settings
$30 = 24000.000  // Max spindle RPM → spindle.maxRPM = 24000.0f
$31 = 0.000      // Min spindle RPM → spindle.minRPM = 0.0f
$32 = 0          // Laser mode → spindle.laserMode = false
```

## Application-Wide Machine Awareness

### 1. Machine Visualization Panel
```cpp
// Automatically uses real machine workspace bounds
void UpdateWorkspaceFromActiveMachine() {
    float maxX = MachineConfigManager::Instance().GetWorkspaceX(activeId);
    float maxY = MachineConfigManager::Instance().GetWorkspaceY(activeId);
    float maxZ = MachineConfigManager::Instance().GetWorkspaceZ(activeId);
    
    SetWorkspaceFromMachine(true, 0, maxX, 0, maxY, -maxZ, 0);
}
```

### 2. Jog Panel  
```cpp
// Limits jog distances and feed rates to machine capabilities
void UpdateJoggingLimits() {
    auto config = MachineConfigManager::Instance().GetActiveMachine();
    
    m_feedRateSlider->SetMax(config.motion.maxRateX);
    m_maxJogDistance = std::min({config.motion.maxTravelX, 
                                config.motion.maxTravelY}) / 2.0f;
    m_homeButton->Enable(config.homing.enabled);
}
```

### 3. G-Code Editor
```cpp
// Validates G-code against machine limits in real-time
void ValidateGCodeAgainstMachine() {
    auto config = MachineConfigManager::Instance().GetActiveMachine();
    
    // Check coordinates against workspace limits
    if (x > config.motion.maxTravelX) {
        HighlightError("X coordinate exceeds machine limit");
    }
    
    // Check spindle speeds against machine limits
    if (spindleRPM > config.spindle.maxRPM) {
        HighlightError("Spindle speed exceeds machine limit");  
    }
}
```

### 4. Macro Panel
```cpp
// Creates machine-specific macros automatically
void CreateMachineSpecificMacros() {
    auto config = MachineConfigManager::Instance().GetActiveMachine();
    
    // Only show homing macros if machine supports homing
    if (config.homing.enabled) {
        AddMacroButton("Home All", "$H", "Home all axes");
    }
    
    // Only show probe macros if machine has probe
    if (config.probe.enabled) {
        AddMacroButton("Probe Z", "G38.2 Z-10 F100", "Probe downward");
    }
    
    // Create workspace corner navigation
    wxString cornerCmd = wxString::Format("G0 X%.1f Y%.1f", 
                                         config.motion.maxTravelX,
                                         config.motion.maxTravelY);
    AddMacroButton("Go to Corner", cornerCmd, "Move to workspace corner");
}
```

### 5. DRO Panel
```cpp
// Shows machine-specific information and capabilities
void UpdateMachineInfo() {
    auto config = MachineConfigManager::Instance().GetActiveMachine();
    
    m_machineNameLabel->SetLabel(config.name);
    m_firmwareLabel->SetLabel(config.firmware.version);
    
    wxString caps = wxString::Format(
        "Workspace: %.0fx%.0fx%.0f | Max Feed: %.0f | Spindle: %.0f RPM | %s%s",
        config.motion.maxTravelX, config.motion.maxTravelY, config.motion.maxTravelZ,
        config.motion.maxRateX, config.spindle.maxRPM,
        config.homing.enabled ? "Homing+" : "Homing-",
        config.probe.enabled ? " Probe+" : " Probe-"
    );
    m_capabilitiesLabel->SetLabel(caps);
}
```

## JSON Storage Format

Complete machine configurations stored in `machines.json`:

```json
{
  "machines": [
    {
      "id": "cnc_router_01",
      "name": "CNC Router",
      "host": "192.168.1.100",
      "port": 23,
      "motion": {
        "maxTravelX": 300.0,
        "maxTravelY": 200.0, 
        "maxTravelZ": 100.0,
        "maxRateX": 3000.0,
        "maxRateY": 3000.0,
        "maxRateZ": 500.0,
        "accelX": 250.0,
        "accelY": 250.0,
        "accelZ": 50.0
      },
      "homing": {
        "enabled": true,
        "feedRate": 500.0,
        "seekRate": 2500.0,
        "pullOff": 1.0
      },
      "spindle": {
        "maxRPM": 24000.0,
        "minRPM": 0.0,
        "laserMode": false
      },
      "probe": {
        "enabled": true,
        "feedRate": 100.0,
        "seekDistance": 10.0
      },
      "firmware": {
        "version": "FluidNC v3.7.0",
        "buildDate": "2023-10-15",
        "boardType": "ESP32"
      },
      "grblSettings": {
        "130": 300.0,
        "131": 200.0,
        "132": 100.0
        // ... all other settings
      },
      "capabilitiesDiscovered": true,
      "lastDiscovery": "2025-08-13T12:30:00"
    }
  ]
}
```

## Benefits

### For Users:
- **One-Click Configuration**: Auto-discovery eliminates manual setup
- **Machine-Specific UI**: Every panel adapts to machine capabilities  
- **Safety**: Automatic validation prevents exceeding machine limits
- **Precision**: Real workspace bounds, feed rates, spindle limits
- **Convenience**: Machine-aware macros and shortcuts

### For Developers:  
- **Single Source of Truth**: All machine data in one place
- **Easy Access**: Simple API for any panel to get machine info
- **Maintainable**: Clear separation between discovery and usage
- **Extensible**: Easy to add new machine capabilities
- **Consistent**: Same machine data used throughout application

## Implementation Priority

### Phase 1: Foundation ✅
- MachineVisualizationPanel workspace bounds control
- Basic MachineConfigManager structure  
- Workspace hiding/showing logic

### Phase 2: Enhanced Dialog 🚧  
- Multi-tab Edit Machine Dialog
- Auto-discovery button and logic
- FluidNC response parsing
- Complete settings storage

### Phase 3: Application Integration 📋
- Update all panels to use MachineConfigManager
- Machine-aware UI adaptations
- G-code validation against machine limits
- Machine-specific macro generation

### Phase 4: Advanced Features 🚀
- Real-time settings synchronization  
- Machine capability comparison
- Profile import/export
- Multi-machine session management

This architecture transforms the application from a generic G-code sender into an **intelligent, machine-aware CNC control system** that automatically adapts to any connected machine's specific capabilities and limitations.
