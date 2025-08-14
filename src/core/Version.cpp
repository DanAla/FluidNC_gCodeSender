/**
 * core/Version.cpp
 * Version information implementation
 */

#include "Version.h"
#include "BuildCounter.h"
#include <sstream>

namespace FluidNC {
namespace Version {

std::string GetFullVersionString() {
    std::ostringstream oss;
    
    // VERSION_STRING is always available from generated_version.h
    oss << APP_NAME << " v" << VERSION_STRING;
    
    // Add build type if debug
    if (std::string(BUILD_CONFIG) == "Debug") {
        oss << " (Debug)";
    }
    
    return oss.str();
}

std::string GetBuildInfoString() {
    std::ostringstream oss;
    oss << "Built: " << BUILD_INFO << "\n";
    oss << "Git Version: " << GIT_TAG << "\n";
    oss << BuildCounter::GetBuildCountString() << "\n";
    oss << "Platform: " << PLATFORM << " " << ARCHITECTURE << "\n";
    oss << "Compiler: " << COMPILER << " " << GCC_VERSION << "\n";
    oss << "Commit: " << COMMIT_HASH << "\n";
    oss << "Branch: " << BRANCH << "\n\n";
    
    // Dependency versions
    oss << "Dependencies:\n";
    oss << "  * wxWidgets: " << WXWIDGETS_VER << "\n";
    oss << "  * nlohmann/json: " << JSON_VER << "\n";
    oss << "  * CMake: " << CMAKE_VER << "\n";
    oss << "  * MinGW Target: " << MINGW_ARCH << "\n";
    oss << "  * C++ Standard: 17";
    
    return oss.str();
}

std::string GetAboutInfoString() {
    std::ostringstream oss;
    
    // Main version info
    oss << GetFullVersionString() << "\n\n";
    
    // Description
    oss << "Professional CNC Control Application\n";
    oss << "Built with C++ and wxWidgets\n\n";
    
    // Features
    oss << GetFeaturesString() << "\n\n";
    
    // Repository and build info
    oss << "Repository: " << REPOSITORY_URL << "\n";
    oss << "Issues: " << ISSUES_URL << "\n\n";
    
    // Build details
    oss << GetBuildInfoString();
    
    return oss.str();
}

std::string GetFeaturesString() {
    return "- Supports multiple CNC machines via Telnet, USB, and UART\n"
           "- Real-time position monitoring and G-code execution\n"
           "- SVG file visualization and macro support";
}

} // namespace Version
} // namespace FluidNC
