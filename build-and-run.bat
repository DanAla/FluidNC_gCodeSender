@echo off
echo ========================================
echo    FluidNC gCode Sender - Quick Build
echo ========================================
echo.

if not exist "build-mingw" (
    echo Creating build directory...
    mkdir build-mingw
    cd build-mingw
    echo Configuring project with CMake...
    cmake .. -G "MinGW Makefiles"
    if %ERRORLEVEL% neq 0 (
        echo *** CMAKE CONFIGURATION FAILED ***
        pause
        exit /b 1
    )
    cd ..
)

echo Building in build-mingw directory...
cd build-mingw
mingw32-make -j4

if %ERRORLEVEL% neq 0 (
    echo.
    echo *** BUILD FAILED ***
    pause
    cd ..
    exit /b 1
)

echo.
echo *** BUILD SUCCESSFUL ***
echo Starting FluidNC gCode Sender...
echo.
FluidNC_gCodeSender.exe

cd ..
echo.
echo Done.
pause
