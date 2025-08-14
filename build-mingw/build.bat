@echo off
echo FluidNC gCode Sender - Build Script
echo ===================================

REM Check for --full flag
set FULL_REBUILD=0
if /i "%1" == "--full" (
    echo "Full rebuild requested."
    set FULL_REBUILD=1
    REM Clean CMake cache and files to force full reconfigure and rebuild
    if exist "CMakeCache.txt" del "CMakeCache.txt"
    if exist "CMakeFiles" rd /s /q "CMakeFiles"
    if exist "cmake_install.cmake" del "cmake_install.cmake"
    if exist "Makefile" del "Makefile"
)

REM Kill any running instances of the executable
echo Checking for running FluidNC_gCodeSender.exe...
taskkill /f /im FluidNC_gCodeSender.exe >nul 2>&1
if %errorlevel% equ 0 (
    echo Terminated running FluidNC_gCodeSender.exe
) else (
    echo No running instances found
)

REM We are already in the build directory, so configure CMake if needed
REM The check for CMakeCache.txt will now trigger after a --full clean
if not exist "CMakeCache.txt" (
    echo Configuring CMake...
    cmake -G "MinGW Makefiles" .
    if %errorlevel% neq 0 (
        echo CMake configuration failed!
        exit /b 1
    )
    echo CMake configuration completed.
)

echo Building project...
mingw32-make
if %errorlevel% neq 0 (
    echo Build failed!
    exit /b 1
)

echo Build completed successfully!
echo Executable location: %cd%\FluidNC_gCodeSender.exe

REM Auto-run the executable as per rules
if exist "FluidNC_gCodeSender.exe" (
    echo Starting application...
    start "" "FluidNC_gCodeSender.exe"
)
