cmake_minimum_required(VERSION 3.16)
project(FluidNC_gCodeSender VERSION 1.0.0 LANGUAGES CXX RC)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets - point to our specific build (using working template)
set(wxWidgets_ROOT_DIR "C:/wxWidgets-3.3.1")
set(wxWidgets_LIB_DIR "C:/wxWidgets-3.3.1/build-mingw/lib/gcc_x64_lib")
set(wxWidgets_CONFIGURATION mswu)
find_package(wxWidgets REQUIRED COMPONENTS core base adv)

# Manually set wxWidgets libraries and includes for MinGW (WORKING TEMPLATE)
set(wxWidgets_LIBRARIES 
    "${wxWidgets_LIB_DIR}/libwxmsw33u_core.a"
    "${wxWidgets_LIB_DIR}/libwxbase33u.a"
    "${wxWidgets_LIB_DIR}/libwxmsw33u_adv.a"
    "${wxWidgets_LIB_DIR}/libwxtiff.a"
    "${wxWidgets_LIB_DIR}/libwxjpeg.a"
    "${wxWidgets_LIB_DIR}/libwxpng.a"
    "${wxWidgets_LIB_DIR}/libwxzlib.a"
    "${wxWidgets_LIB_DIR}/libwxregexu.a"
    "${wxWidgets_LIB_DIR}/libwxexpat.a"
    gdiplus kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 
    uuid odbc32 odbccp32 comctl32 rpcrt4 winmm uxtheme shlwapi version msimg32 oleacc
)
set(wxWidgets_INCLUDE_DIRS 
    "C:/wxWidgets-3.3.1/include"
    "C:/wxWidgets-3.3.1/build-mingw/lib/gcc_x64_lib/mswu"
)
set(wxWidgets_DEFINITIONS -D__WXMSW__)

include_directories(${wxWidgets_INCLUDE_DIRS})
add_definitions(${wxWidgets_DEFINITIONS})

# Use single-header nlohmann/json approach
if(EXISTS "${CMAKE_SOURCE_DIR}/external/json.hpp")
    message(STATUS "Using single-header nlohmann/json")
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE ${CMAKE_SOURCE_DIR}/external)
    # Create alias for consistency
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
else()
    message(STATUS "Looking for system nlohmann/json")
    find_package(nlohmann_json REQUIRED)
endif()

# Threading support
find_package(Threads REQUIRED)

# Platform-specific settings
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE ON)
endif()

# Source files - Minimal for testing
set(CORE_SOURCES
    # src/core/StateManager.cpp  # Temporarily disabled
    # src/core/ConnectionManager.cpp # Temporarily disabled
    src/core/Logger.cpp
    # src/core/FluidNCClient.cpp
    # src/core/GCodeGenerator.cpp
    # src/core/SVGLoader.cpp
    # src/core/MacroEngine.cpp
    # src/core/GCodeEditor.cpp
)

set(GUI_SOURCES
    src/gui/MainFrame.cpp
    src/gui/WelcomeDialog.cpp
    # src/gui/DROPanel.cpp
    # src/gui/JogPanel.cpp
    # src/gui/SettingsPanel.cpp
    # src/gui/SettingsDialog.cpp
    # src/gui/TelnetSetupPanel.cpp
    # src/gui/SVGViewer.cpp
    # src/gui/GCodePanel.cpp
    # src/gui/MacroPanel.cpp
)

set(APP_SOURCES
    src/main.cpp
    src/App.cpp
)

# Windows resource file
if(WIN32)
    set(RESOURCE_FILES src/app.rc)
endif()

# Include directories
include_directories(src)
include_directories(src/core)
include_directories(src/gui)

# Create executable
add_executable(${PROJECT_NAME} 
    ${CORE_SOURCES} 
    ${GUI_SOURCES} 
    ${APP_SOURCES}
    ${RESOURCE_FILES}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    ${wxWidgets_LIBRARIES}
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Windows specific settings (from working template)
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    # MinGW specific: ensure proper linking
    if(MINGW)
        target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++)
    endif()
endif()

# Copy resources
file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})

# Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY resources DESTINATION share/${PROJECT_NAME})
