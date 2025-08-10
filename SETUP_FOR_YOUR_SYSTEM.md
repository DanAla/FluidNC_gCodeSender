# FluidNC_gCodeSender - Setup for Your MinGW System

## 🎯 **Optimized for Your Proven Configuration**

Based on your **BUILD_GUIDE_MINGW.md**, I've configured the project to work seamlessly with your existing setup:

- ✅ **MinGW-w64 GCC 13.1.0** - Modern C++17/20 support
- ✅ **CMake 3.30.5** - Latest CMake with excellent MinGW support  
- ✅ **wxWidgets 3.3.1** - Source build at `C:\wxWidgets-3.3.1`
- ✅ **Single-header nlohmann/json** - Downloaded automatically

## 🚀 **Quick Start (Using Your Setup)**

### **Prerequisites Verification:**
```powershell
# 1. Check if wxWidgets environment is set
echo $env:wxWidgets_ROOT_DIR
# Should show: C:\wxWidgets-3.3.1

# 2. If not set, add it:
$env:wxWidgets_ROOT_DIR = "C:\wxWidgets-3.3.1"
[Environment]::SetEnvironmentVariable("wxWidgets_ROOT_DIR", "C:\wxWidgets-3.3.1", "User")

# 3. Verify wxWidgets is built for MinGW
ls C:\wxWidgets-3.3.1\build-mingw\lib\libwx_mswu_core-3.3.a
# Should exist from your previous builds
```

### **Build FluidNC_gCodeSender:**
```powershell
# Use the optimized build script
.\build_mingw.bat
```

## 🔧 **What I've Configured for Your System:**

### **1. CMakeLists.txt Adaptations:**
```cmake
# MinGW-specific optimizations
if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3")
endif()

# Your proven wxWidgets setup
set(wxWidgets_ROOT_DIR $ENV{wxWidgets_ROOT_DIR})
if(MINGW AND wxWidgets_ROOT_DIR)
    set(wxWidgets_LIB_DIR ${wxWidgets_ROOT_DIR}/build-mingw/lib)
    set(wxWidgets_INCLUDE_DIRS ${wxWidgets_ROOT_DIR}/include ${wxWidgets_ROOT_DIR}/build-mingw/lib/wx/include/msw-unicode-static-3.3)
endif()

# Single-header nlohmann/json (your preferred approach)
if(EXISTS "${CMAKE_SOURCE_DIR}/external/json.hpp")
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE ${CMAKE_SOURCE_DIR}/external)
endif()
```

### **2. Automated Dependency Management:**
- **nlohmann/json** - Downloaded automatically if missing
- **wxWidgets detection** - Uses your `build-mingw` directory
- **MinGW optimization** - Compiler flags tuned for your setup

### **3. Build Script Features:**
- **Environment validation** - Checks `wxWidgets_ROOT_DIR`
- **Dependency verification** - Ensures wxWidgets is built
- **Automatic downloads** - Gets nlohmann/json if needed
- **Error diagnostics** - Clear messages for common issues

## 📊 **Build Process Verification:**

### **Expected Output:**
```
✅ Using wxWidgets at: C:\wxWidgets-3.3.1
✅ wxWidgets build found
✅ nlohmann/json header found
🔧 Configuring with CMake (MinGW)...
-- Using single-header nlohmann/json
-- Found wxWidgets: TRUE (found components: core base aui propgrid)
🔨 Building project...
✅ Build successful!
```

## 🚧 **Current Implementation Status:**

### **✅ Ready Components:**
- **StateManager** - Complete implementation with JSON persistence
- **FluidNCClient** - Multi-threaded telnet client
- **Build system** - Fully configured for your MinGW setup
- **Project structure** - Professional C++ organization

### **🚧 Next Implementation Priority:**
1. **MainFrame.cpp** - Professional AUI-based main window
2. **DROPanel.cpp** - Multi-machine coordinate display  
3. **SettingsDialog.cpp** - Comprehensive 4-tab settings
4. **ConnectionManager.cpp** - Multi-protocol connection management

## 🎯 **Testing the Current Foundation:**

### **Step 1: Test Build System**
```powershell
# This will test CMake configuration and dependency detection
.\build_mingw.bat
```

**Expected Result:** Should configure successfully but fail at linking (expected - GUI stubs not implemented yet)

### **Step 2: Verify Configuration**
```powershell
# Check generated build files
ls build-mingw
# Should contain: CMakeCache.txt, Makefile, etc.

# Verify JSON header
ls external/json.hpp
# Should exist and be ~1MB
```

## 📋 **Manual Build Alternative:**

If you prefer your exact process from BUILD_GUIDE_MINGW.md:

```powershell
# Ensure nlohmann/json is available
if (!(Test-Path "external/json.hpp")) {
    Invoke-WebRequest -Uri "https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp" -OutFile "external/json.hpp"
}

# Configure (your proven method)
cmake -G "MinGW Makefiles" -S . -B build-mingw -DCMAKE_BUILD_TYPE=Release

# Build (your proven method)  
mingw32-make -C build-mingw -j4
```

## 🔧 **Troubleshooting Guide:**

### **Common Issues & Solutions:**

#### **Issue: "wxWidgets not found"**
```powershell
# Solution: Set environment variable
$env:wxWidgets_ROOT_DIR = "C:\wxWidgets-3.3.1"
[Environment]::SetEnvironmentVariable("wxWidgets_ROOT_DIR", "C:\wxWidgets-3.3.1", "User")
```

#### **Issue: "libwx_mswu_core-3.3.a not found"**
```powershell
# Solution: Rebuild wxWidgets for MinGW
cd C:\wxWidgets-3.3.1
mkdir build-mingw -Force
cd build-mingw
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DwxBUILD_SHARED=OFF -DwxUSE_STL=ON -DwxUSE_UNICODE=ON ..
mingw32-make -j8
```

#### **Issue: "json.hpp not found"**
```powershell
# Solution: Download manually
mkdir external -Force
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp" -OutFile "external/json.hpp"
```

## 🎯 **Next Steps:**

### **Ready for Implementation:**
The project is now **perfectly configured** for your MinGW system. The next phase is implementing the GUI components:

1. **Run `.\build_mingw.bat`** to verify the foundation
2. **Implement MainFrame.cpp** with AUI docking
3. **Build DROPanel.cpp** with multi-machine support
4. **Create SettingsDialog.cpp** with comprehensive tabs

Your proven MinGW + wxWidgets setup is now fully integrated with our professional CNC control application architecture! 🎯✨
