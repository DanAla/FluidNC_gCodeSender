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
    oss << APP_NAME << " v" << VERSION_STRING_STR;
    
    // Add build type if debug
    if (std::string(BUILD_CONFIG) == "Debug") {
        oss << " (Debug)";
    }
    
    return oss.str();
}

std::string GetBuildInfoString() {
    std::ostringstream oss;
    oss << "Built: " << BUILD_INFO << "\n";
    oss << "Git Version: " << DESCRIBE << "\n";
    oss << BuildCounter::GetBuildCountString() << "\n";
    oss << "Platform: " << PLATFORM << " " << ARCHITECTURE << "\n";
    oss << "Compiler: " << COMPILER << "\n";
    oss << "Commit: " << COMMIT_HASH << "\n";
    oss << "Branch: " << BRANCH;
    
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
    oss << "Supports multiple CNC machines via Telnet, USB, and UART\n";
    oss << "Real-time position monitoring and G-code execution\n";
    oss << "SVG file visualization and macro support\n\n";
    
    // Repository and build info
    oss << "Repository: " << REPOSITORY_URL << "\n";
    oss << "Issues: " << ISSUES_URL << "\n\n";
    
    // Build details
    oss << GetBuildInfoString();
    
    return oss.str();
}

} // namespace Version
} // namespace FluidNC
