# Machine Capability Discovery & Dynamic Workspace Bounds

## Problem Solved
The visualization panel previously displayed a hardcoded gray rectangle (workspace bounds) regardless of machine connection status or actual machine capabilities. This has been resolved with a dynamic system that:

1. **Hides workspace bounds when no machine is connected**
2. **Automatically discovers and displays real machine workspace dimensions** 
3. **Stores machine capabilities persistently** for future sessions

## Architecture Overview

### 1. MachineVisualizationPanel (Enhanced)
**New Features:**
- `SetWorkspaceFromMachine()` - Updates workspace bounds from real machine data
- `HideWorkspaceBounds()` / `ShowWorkspaceBounds()` - Control workspace visibility
- Workspace bounds now hidden by default until machine provides real dimensions

**When Connected:** Shows accurate workspace rectangle based on machine's actual travel limits
**When Disconnected:** Clean visualization without inappropriate workspace bounds

### 2. MachineManagerPanel (Enhanced)
**New Data Structure:**
```cpp
struct MachineConfig {
    // ... existing fields ...
    
    struct MachineCapabilities {
        float workspaceX, workspaceY, workspaceZ;  // Max travel distances
        float maxFeedRate, maxSpindleRPM;
        int numAxes;
        bool hasHoming, hasProbe;
        std::string firmwareVersion, buildInfo;
        bool capabilitiesValid;  // True when queried from machine
    } capabilities;
};
```

**New Methods:**
- `QueryMachineCapabilities()` - Sends FluidNC discovery commands
- `UpdateMachineCapabilities()` - Stores discovered capabilities
- `NotifyVisualizationPanel()` - Updates workspace bounds in visualization

### 3. Machine Discovery Workflow

#### When Machine Connects:
1. **Connection Success** → MachineManagerPanel receives connection event
2. **Query Capabilities** → Send FluidNC commands:
   - `$$` - Get all machine settings (includes axis limits)
   - `$I` - Get build/version information  
   - `$G` - Get parser state
   - `$#` - Get coordinate system data

3. **Parse Responses** → Extract key settings:
   - `$130` = X-axis maximum travel (mm)
   - `$131` = Y-axis maximum travel (mm) 
   - `$132` = Z-axis maximum travel (mm)
   - `$110-$112` = Maximum feed rates per axis
   - `$23` = Homing enable flag
   - Build version and hardware info

4. **Store Capabilities** → Update machine configuration and save to JSON
5. **Update Visualization** → Call `SetWorkspaceFromMachine()` with real bounds
6. **Update UI** → Show discovered capabilities in machine details

#### When Machine Disconnects:
1. **Connection Lost** → MachineManagerPanel receives disconnect event
2. **Hide Workspace** → Call `SetWorkspaceFromMachine(false)` to hide bounds
3. **Preserve Data** → Keep discovered capabilities in saved configuration

## FluidNC Command Reference

### Discovery Commands
```
$$     - Get all machine settings
$I     - Get build info (version, date, etc.)  
$G     - Get current G-code parser state
$#     - Get coordinate system offsets
?      - Get real-time status
```

### Key Settings to Extract
```
$130   - X-axis maximum travel (mm)
$131   - Y-axis maximum travel (mm)
$132   - Z-axis maximum travel (mm)
$110   - X-axis maximum rate (mm/min)
$111   - Y-axis maximum rate (mm/min)
$112   - Z-axis maximum rate (mm/min)
$23    - Homing enable (0=disable, 1=enable)
$27    - Homing pull-off distance (mm)
```

### Example Response Parsing
```cpp
// From $$ command:
$130=300.000 (X-axis maximum travel, millimeters)
$131=200.000 (Y-axis maximum travel, millimeters)  
$132=100.000 (Z-axis maximum travel, millimeters)

// Results in:
capabilities.workspaceX = 300.0f;
capabilities.workspaceY = 200.0f; 
capabilities.workspaceZ = 100.0f;
```

## JSON Storage Format

Machine capabilities are stored persistently in `machines.json`:

```json
{
  "machines": [
    {
      "id": "machine1",
      "name": "CNC Router", 
      "host": "192.168.1.100",
      "port": 23,
      "capabilities": {
        "workspaceX": 300.0,
        "workspaceY": 200.0, 
        "workspaceZ": 100.0,
        "maxFeedRate": 3000.0,
        "maxSpindleRPM": 24000.0,
        "numAxes": 3,
        "hasHoming": true,
        "hasProbe": false,
        "firmwareVersion": "FluidNC v3.7.0", 
        "buildInfo": "FluidNC [VER:3.7.0] [Build:2023-10-15]",
        "capabilitiesValid": true
      }
    }
  ]
}
```

## Benefits

### For Users:
- **Accurate Visualization**: See the actual machine workspace, not a generic rectangle
- **Automatic Configuration**: No manual entry of machine dimensions required
- **Clean Interface**: No confusing workspace bounds when no machine is connected
- **Persistent Memory**: Capabilities remembered between sessions

### For Developers:
- **Extensible**: Easy to add new capability discovery features
- **Maintainable**: Clear separation between discovery, storage, and visualization
- **Reliable**: Graceful handling of connection failures and missing data

## Implementation Status

### ✅ Completed:
- MachineVisualizationPanel workspace bounds control
- Enhanced MachineConfig data structure
- Workspace hiding/showing logic
- Example implementation framework

### 🚧 To Implement:
- FluidNC response parsing logic
- Integration with CommunicationManager callbacks
- Machine capability query methods
- UI updates to display discovered capabilities
- Comprehensive error handling for malformed responses

## Future Enhancements

1. **Advanced Capabilities**: Probe detection, spindle VFD detection, tool changer support
2. **Visual Feedback**: Show homing switches, probe points, axis directions in visualization
3. **Soft Limits**: Use discovered limits to prevent G-code from exceeding machine bounds
4. **Performance Optimization**: Cache frequently-used capabilities, batch command sending
5. **Multi-Machine Support**: Handle capabilities for multiple connected machines simultaneously

This architecture provides a solid foundation for intelligent machine discovery while maintaining clean separation of concerns and extensibility for future enhancements.
