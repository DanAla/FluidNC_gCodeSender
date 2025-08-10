# FluidNC_gCodeSender - C++ Version

Cross-platform GUI for FluidNC: SVG → G-code, live DRO, jogging, macros, auto-restore after power loss.

## Project Status

This is the **C++ wxWidgets conversion** of the original Python-based FluidNC_gCodeSender. The conversion is currently **in progress**.

### ✅ Completed Components

#### Core Infrastructure
- **StateManager**: JSON-based persistent state management using `nlohmann::json`
  - Thread-safe configuration storage
  - Automatic background saves every 5 seconds
  - Nested key support (e.g., "telnet/host")
  - Compatible with original Python JSON format

- **FluidNCClient**: Asynchronous telnet communication with FluidNC
  - Multi-threaded design (separate RX/TX threads)
  - Real-time DRO (Machine/Work position) parsing
  - Command queuing with auto-retry on disconnect
  - Cross-platform socket implementation (Windows/Linux)
  - Auto-reconnection with configurable intervals

#### Build System
- **CMake**: Modern CMake configuration with dependency management
- **Dependencies**: wxWidgets, nlohmann::json, threading support
- **Cross-platform**: Windows (MSVC/MinGW) and Linux support

### 🚧 In Progress Components

#### GUI Framework
- **MainFrame**: AUI-based dockable panel interface (header created)
- **DROPanel**: Real-time coordinate display + manual G-code input (header created)
- **JogPanel**: Machine jogging and homing controls (header created)
- **TelnetSetupPanel**: Connection configuration interface
- **SettingsPanel**: Property grid for all application settings

### ❌ Planned Components

#### Core Features
- **GCodeGenerator**: SVG to G-code conversion engine
- **SVGLoader**: SVG file parsing and manipulation
- **MacroEngine**: Custom macro system with scripting
- **GCodeEditor**: Interactive G-code editing and reordering

#### GUI Panels
- **SVGViewer**: SVG display with toolpath preview overlay
- **GCodePanel**: Tree view of G-code operations with drag-drop reordering
- **MacroPanel**: Macro management and execution interface

## Architecture Comparison

### Python Version Architecture
```
main.py
├── gui/
│   ├── main_frame.py (wxPython + AUI)
│   ├── dro_panel.py
│   ├── jog_panel.py
│   └── ...
└── core/
    ├── fluidnc_client.py (threading + socket)
    ├── state_manager.py (JSON + threading)
    └── ...
```

### C++ Version Architecture
```
src/main.cpp
├── gui/
│   ├── MainFrame.{h,cpp} (wxWidgets + AUI)
│   ├── DROPanel.{h,cpp}
│   ├── JogPanel.{h,cpp}
│   └── ...
└── core/
    ├── FluidNCClient.{h,cpp} (std::thread + sockets)
    ├── StateManager.{h,cpp} (nlohmann::json + std::mutex)
    └── ...
```

## Key Design Decisions

### 1. **JSON Compatibility**
The C++ version uses `nlohmann::json` which is 100% compatible with Python's JSON format, ensuring seamless migration of existing configuration files.

### 2. **Thread-Safe Design**
- **StateManager**: Uses `std::recursive_mutex` for thread-safe access
- **FluidNCClient**: Separate RX/TX threads with proper synchronization
- **GUI Updates**: Uses wxWidgets event system for cross-thread communication

### 3. **Modern C++ Features**
- C++17 standard with `std::filesystem`, `std::thread`, smart pointers
- RAII resource management
- Template-based configuration system

### 4. **Cross-Platform Compatibility**
- Socket abstraction for Windows (Winsock) and Linux (POSIX)
- CMake build system with automatic dependency detection
- Platform-specific compilation flags

## Building

### Prerequisites
- **CMake** 3.16+
- **C++17** compatible compiler (MSVC 2019+, GCC 7+, Clang 5+)
- **wxWidgets** 3.1+ with AUI and PropGrid
- **nlohmann::json** (can be installed via package manager)

### Windows (MSVC)
```bash
# Install dependencies via vcpkg (recommended)
vcpkg install wxwidgets nlohmann-json

# Configure and build
cmake -B build-win -S . -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build-win --config Release
```

### Linux (Ubuntu/Debian)
```bash
# Install dependencies
sudo apt-get install libwxgtk3.0-gtk3-dev nlohmann-json3-dev cmake build-essential

# Build
cmake -B build-linux -S .
cmake --build build-linux
```

## Configuration

The application maintains the same configuration format as the Python version:
- **Settings**: `config/settings.json` - Main application configuration
- **Recovery**: `config/recovery.json` - Auto-saved state for crash recovery

### Example Configuration
```json
{
  "telnet": {
    "host": "192.168.1.100",
    "port": 23,
    "connect_retries": 3,
    "retry_interval_s": 2
  },
  "window_geometry": [100, 100, 1200, 800],
  "aui_layout": {
    "perspective": "layout0|name=setup;caption=;state=..."
  },
  "gcode_history": [
    "G0 X10 Y10",
    "$H",
    "G1 X0 Y0 F1000"
  ]
}
```

## Migration from Python Version

1. **Backup your configuration**:
   ```bash
   cp -r config/ config_backup/
   ```

2. **The C++ version will automatically read existing settings**:
   - All telnet settings
   - Window positions and layout
   - G-code command history
   - Macro definitions (when implemented)

3. **New features in C++ version**:
   - Better performance due to native compilation
   - Lower memory usage
   - Improved thread safety
   - Enhanced error handling

## Development Progress Tracking

### Phase 1: Core Infrastructure ✅
- [x] CMake build system
- [x] StateManager (JSON persistence)
- [x] FluidNCClient (network communication)
- [x] Cross-platform socket abstraction

### Phase 2: Basic GUI 🚧
- [ ] MainFrame implementation
- [ ] DROPanel implementation  
- [ ] JogPanel implementation
- [ ] TelnetSetupPanel implementation
- [ ] SettingsPanel implementation

### Phase 3: Advanced Features ❌
- [ ] SVG loading and display
- [ ] G-code generation from SVG
- [ ] Macro engine
- [ ] G-code editor with drag-drop
- [ ] Toolpath preview

### Phase 4: Polish & Testing ❌
- [ ] Comprehensive error handling
- [ ] Unit tests
- [ ] Documentation
- [ ] Installer/packaging
- [ ] Python code removal

## Contributing

The conversion is following the original Python code structure closely while modernizing with C++ best practices. Key areas needing attention:

1. **GUI Implementation**: Converting wxPython panels to wxWidgets C++
2. **SVG Processing**: Need to find suitable C++ SVG library (consider Cairo, NanoSVG)
3. **Advanced Features**: Macro scripting engine, G-code manipulation
4. **Testing**: Unit tests and integration tests
5. **Documentation**: API documentation and user guides

## License

BSD 3-Clause License (same as original Python version)
