# Kinematics-Aware Machine Configuration & Homing

## The CoreXY Challenge

Your CoreXY machine highlights a critical issue that most generic G-code senders ignore: **different kinematics require different homing strategies**.

### Why CoreXY Homing is Different

**Cartesian Machine (Standard):**
- Each motor controls one axis independently
- `$H` homes all axes simultaneously - works perfectly
- No motor interference during homing

**CoreXY Kinematics:**
- **Both X and Y motors move for ANY X or Y movement**
- **Cannot home X and Y simultaneously** - motors fight each other
- **Must home sequentially**: Z first (safe), then X, then Y
- **Proper sequence**: `$HZ` → `$HX` → `$HY`

## Enhanced Machine Configuration

### Kinematics-Aware Homing Settings
```cpp
struct HomingSettings {
    bool enabled = true;
    float feedRate = 500.0f;
    float seekRate = 2500.0f;
    float pullOff = 1.0f;
    
    // NEW: Kinematics-specific homing sequences
    enum HomingSequence {
        SIMULTANEOUS,   // Standard Cartesian: $H
        SEQUENTIAL_ZXY, // CoreXY: $HZ → $HX → $HY  
        SEQUENTIAL_ZYX, // Alternative: $HZ → $HY → $HX
        CUSTOM         // User-defined sequence
    };
    
    HomingSequence sequence = SIMULTANEOUS;
    std::vector<std::string> customSequence; // For complex kinematics
};
```

## Machine-Specific Homing Sequences

### 1. **Cartesian (Standard)**
```
Sequence: SIMULTANEOUS
Commands: $H
Result: All axes home at once - fast and efficient
```

### 2. **CoreXY (Your Machine)**  
```
Sequence: SEQUENTIAL_ZXY
Commands: $HZ → $HX → $HY
Reason: Prevents motor interference between X/Y axes
Safety: Z homes first to clear workspace
```

### 3. **Delta Printers**
```
Sequence: SIMULTANEOUS  
Commands: $H
Reason: All three towers move together naturally
Safety: Homes to top position away from bed
```

### 4. **SCARA Arms**
```
Sequence: SEQUENTIAL_ZXY or CUSTOM
Commands: Custom sequence based on arm geometry  
Reason: Joint angles must be homed in specific order
```

### 5. **Custom Kinematics**
```
Sequence: CUSTOM
Commands: User-defined sequence
Example: ["$HZ", "$HX", "G4 P1000", "$HY", "$HA"] 
Includes: Delays, multiple axes, special commands
```

## Auto-Discovery Enhancement

### Kinematics Detection
The **"Query Machine"** button should also detect kinematics:

```cpp
// During auto-discovery, detect machine type
std::string DetectKinematics(const std::map<int, float>& grblSettings) {
    // Check FluidNC kinematics setting (if available)
    if (grblSettings.find(400) != grblSettings.end()) {
        int type = static_cast<int>(grblSettings[400]);
        switch (type) {
            case 0: return "Cartesian";
            case 1: return "CoreXY"; 
            case 2: return "Delta";
            case 3: return "SCARA";
        }
    }
    
    // Check build info for kinematics hints
    // Parse $I response for "CoreXY", "Delta", etc.
    
    return "Cartesian"; // Safe default
}

// Auto-configure based on detected kinematics
void AutoConfigureHoming(MachineConfig& config, const std::string& kinematics) {
    if (kinematics == "CoreXY") {
        config.homing.sequence = SEQUENTIAL_ZXY;
    } else if (kinematics == "Delta") {
        config.homing.sequence = SIMULTANEOUS;
    } else {
        config.homing.sequence = SIMULTANEOUS;
    }
}
```

## Smart Homing Manager

### Sequential Command Execution
```cpp
class HomingManager {
public:
    void HomeAllAxes(const std::string& machineId) {
        auto config = MachineConfigManager::Instance().GetMachine(machineId);
        
        std::vector<std::string> commands;
        switch (config.homing.sequence) {
            case SEQUENTIAL_ZXY:
                commands = {"$HZ", "$HX", "$HY"};
                break;
            case SIMULTANEOUS:
                commands = {"$H"};
                break;
            case CUSTOM:
                commands = config.homing.customSequence;
                break;
        }
        
        // Execute commands sequentially, waiting for "ok" between each
        ExecuteHomingSequence(machineId, commands);
    }
};
```

### Progress Tracking
- **Shows current step**: "Homing Z axis..." → "Homing X axis..." → "Homing Y axis..."
- **Handles errors**: If any step fails, abort sequence and report
- **UI feedback**: Button shows "Homing..." and progress indication

## Edit Machine Dialog Updates

### Homing Tab Enhancement
```
[Homing Settings]
☑ Enable Homing

Homing Sequence: [Dropdown]
├─ Simultaneous (Cartesian)     ← Standard machines
├─ Sequential Z→X→Y (CoreXY)    ← Your machine type  
├─ Sequential Z→Y→X (Alternative)
└─ Custom Sequence...

Feed Rate: [500.0] mm/min
Seek Rate: [2500.0] mm/min  
Pull-off: [1.0] mm

[Test Homing Sequence] [Auto-Detect Kinematics]
```

### Custom Sequence Editor
For complex machines:
```
Custom Homing Sequence:
1. [$HZ     ] ← Home Z axis first
2. [G4 P500 ] ← Wait 500ms 
3. [$HX     ] ← Home X axis
4. [G4 P500 ] ← Wait 500ms
5. [$HY     ] ← Home Y axis

[+] Add Step  [-] Remove Step  [Test Sequence]
```

## UI Adaptation

### Machine-Aware Homing Buttons
```cpp
void UpdateHomingUI() {
    auto config = MachineConfigManager::Instance().GetActiveMachine();
    
    switch (config.homing.sequence) {
        case SIMULTANEOUS:
            m_homeAllButton->SetLabel("Home All");
            m_homeAllButton->SetToolTip("Home all axes simultaneously");
            break;
            
        case SEQUENTIAL_ZXY:
            m_homeAllButton->SetLabel("Home All (Z→X→Y)");
            m_homeAllButton->SetToolTip("Home axes sequentially for CoreXY kinematics");
            break;
            
        case CUSTOM:
            m_homeAllButton->SetLabel("Home All (Custom)");
            m_homeAllButton->SetToolTip("Execute custom homing sequence");
            break;
    }
}
```

### Confirmation Dialogs
```cpp
wxString message;
switch (config.homing.sequence) {
    case SEQUENTIAL_ZXY:
        message = "Home axes in CoreXY sequence (Z→X→Y)?\n\n"
                 "This will:\n"
                 "1. Home Z axis first\n" 
                 "2. Home X axis\n"
                 "3. Home Y axis\n\n"
                 "Each axis will home individually to prevent motor interference.";
        break;
}

wxMessageBox(message, "Confirm CoreXY Homing", wxYES_NO | wxICON_QUESTION);
```

## Benefits for Your CoreXY Machine

### **Before (Generic Homing):**
- Click "Home All" → Sends `$H`
- **X and Y motors fight each other**
- **Homing fails or produces errors**
- **User must manually home each axis individually**

### **After (Kinematics-Aware Homing):**
- Click "Home All (Z→X→Y)" → Automatically sends `$HZ`, waits, `$HX`, waits, `$HY`
- **Proper sequential homing for CoreXY**
- **Reliable, predictable homing every time**
- **No manual intervention required**

## Implementation Priority

### Phase 1: Core Functionality ✅
- Basic workspace bounds fixed (completed)
- Machine configuration structure enhanced

### Phase 2: Homing Enhancement 🚧
- Add `HomingSequence` enum to machine config
- Implement `HomingManager` for sequential execution
- Update Edit Machine Dialog with homing sequence selection

### Phase 3: Auto-Detection 📋
- Kinematics detection from machine responses
- Auto-configuration of appropriate homing sequence
- UI updates for machine-specific homing

### Phase 4: Advanced Features 🚀
- Custom sequence editor for complex kinematics
- Real-time homing progress indication
- Error recovery and retry logic

This kinematics-aware system ensures that **every machine type** - from simple Cartesian to complex CoreXY, Delta, and SCARA - can home properly using the correct sequence for their specific mechanical configuration. Your CoreXY machine will finally have the proper `$HZ → $HX → $HY` sequence it needs! 🤖⚙️
