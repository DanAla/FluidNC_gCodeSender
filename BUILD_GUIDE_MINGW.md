# Complete Build Guide - MinGW + CMake

## 🎯 Your Setup
- ✅ Windows with MinGW-w64 GCC 13.1.0
- ✅ CMake 3.30.5
- ✅ wxWidgets 3.3.1 source installed at C:\wxWidgets-3.3.1
- ✅ Visual Studio (for optional IDE support)

## 📋 Step-by-Step Build Process

### Step 1: Build wxWidgets with MinGW (One-time setup)

```powershell
# Navigate to wxWidgets directory
cd C:\wxWidgets-3.3.1

# Create build directory for MinGW
mkdir build-mingw
cd build-mingw

# Configure wxWidgets with CMake for MinGW
cmake -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DwxBUILD_SHARED=OFF `
  -DwxUSE_STL=ON `
  -DwxUSE_UNICODE=ON `
  ..

# Build wxWidgets (this takes 10-20 minutes)
mingw32-make -j8
```

### Step 2: Set Environment Variable
```powershell
# Add to your environment variables (permanent)
$env:wxWidgets_ROOT_DIR = "C:\wxWidgets-3.3.1"
[Environment]::SetEnvironmentVariable("wxWidgets_ROOT_DIR", "C:\wxWidgets-3.3.1", "User")
```

### Step 3: Get nlohmann/json
```powershell
# Download single header (easiest approach)
cd PerlinNoiseCircle_wxCPP
mkdir external
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp" -OutFile "external/json.hpp"
```

### Step 4: Build Our Project
```powershell
# Configure our project
cmake -G "MinGW Makefiles" -S . -B build-mingw -DCMAKE_BUILD_TYPE=Release

# Build
mingw32-make -C build-mingw -j4
```

## 🔧 Alternative: Simplified CMakeLists.txt

If step 3 works but you get linking issues, we can modify our CMakeLists.txt to use the single header approach.
