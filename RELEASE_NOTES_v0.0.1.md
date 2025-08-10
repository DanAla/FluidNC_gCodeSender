# FluidNC gCode Sender v0.0.1 - Release Notes

**Release Date:** August 10, 2025  
**First Functional C++ Release**

## 🎉 **Major Milestone: Complete Python to C++ Conversion**

This is the first functional release of FluidNC gCode Sender as a native C++ wxWidgets application, completely replacing the original Python implementation.

## ✨ **New Features**

### **Core Application Framework**
- ✅ **Native C++ Implementation** - Complete rewrite using C++ and wxWidgets 3.3.1
- ✅ **Professional GUI Framework** - Modern wxWidgets-based user interface
- ✅ **CMake Build System** - Cross-platform build configuration with MinGW support
- ✅ **Thread-Safe Architecture** - Foundation for multi-threaded CNC communication

### **User Interface**
- ✅ **Main Application Window** - Professional layout with menu bar and status bar
- ✅ **Welcome Dialog System** - Rich welcome dialog with "Don't show again" option
- ✅ **Menu System** - File and Help menus with keyboard shortcuts
- ✅ **Status Bar** - Multi-field status bar for machine status, connection, and position
- ✅ **Settings Persistence** - User preferences saved using wxWidgets config system

### **Build System & Development**
- ✅ **MinGW-w64 Compatibility** - Optimized for Windows MinGW-w64 GCC 13.1.0
- ✅ **Static Linking** - Self-contained executable (~14MB) with no external dependencies
- ✅ **Out-of-Source Builds** - Clean project structure following C++ best practices
- ✅ **Build Scripts** - Convenient build.bat for easy compilation and execution

### **Architecture Foundation**
- ✅ **Modular Design** - Separated GUI, core logic, and utility components
- ✅ **Extensible Framework** - Ready for machine management, G-code processing, and CNC communication
- ✅ **Professional Logging System** - Comprehensive logging with file output and multiple levels
- ✅ **JSON Configuration Support** - Built-in nlohmann/json for settings and state management

## 🔧 **Technical Specifications**

- **Language:** C++17
- **GUI Framework:** wxWidgets 3.3.1
- **Build System:** CMake 3.16+
- **Compiler:** MinGW-w64 GCC 13.1.0
- **Dependencies:** Single-header nlohmann/json, pthread
- **Platform:** Windows (with cross-platform foundation)

## 📁 **Project Structure**
```
FluidNC_gCodeSender/
├── src/                      # C++ source code
│   ├── gui/                  # GUI components
│   ├── core/                 # Core logic and utilities
│   └── main.cpp & App.cpp    # Application entry points
├── external/                 # Third-party dependencies
├── build-mingw/             # Build output directory
│   └── build.bat            # Build and run script
├── CMakeLists.txt           # CMake configuration
└── Documentation files
```

## 🚀 **Getting Started**

### **Prerequisites:**
- MinGW-w64 GCC 13.1.0
- wxWidgets 3.3.1 (built for MinGW)
- CMake 3.16+

### **Building:**
1. Navigate to `build-mingw/` directory
2. Run `build.bat`
3. Application will build and start automatically

### **Running:**
- Execute `FluidNC_gCodeSender.exe` from the build directory
- Welcome dialog will guide you through the application features

## 🔮 **Future Development (v0.1.0+)**

The foundation is now in place for implementing the core CNC functionality:

- **Multi-Machine Management** - Support for multiple CNC machine configurations
- **FluidNC Communication** - Telnet, USB, and UART protocol support  
- **Real-Time DRO Display** - Digital readouts for machine position
- **Advanced Jogging Controls** - Precise machine movement controls
- **G-code Editor & Visualization** - Code editing with syntax highlighting
- **Macro System** - Automation and custom command sequences
- **AUI Docking Interface** - Professional dockable panel system

## 🎯 **Current Status**

This release establishes a solid, professional foundation for the FluidNC gCode Sender application. The application successfully demonstrates:

- Stable C++ wxWidgets application framework
- Professional user interface design
- Robust build system and deployment
- Extensible architecture ready for CNC functionality

---

**For technical details and build instructions, see:**
- `BUILD_GUIDE_MINGW.md` - Complete build setup guide
- `README_CPP.md` - C++ implementation overview
- `REDESIGNED_ARCHITECTURE.md` - Architecture documentation
