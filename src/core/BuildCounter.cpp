/**
 * core/BuildCounter.cpp
 * Build counter management implementation
 */

#include "BuildCounter.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace FluidNC {
namespace BuildCounter {

int GetCurrentBuildCount() {
    std::ifstream file(BUILD_COUNTER_FILE);
    if (!file.is_open()) {
        return 0;
    }
    
    int count = 0;
    file >> count;
    file.close();
    
    // Ensure count is valid (non-negative)
    return (count >= 0) ? count : 0;
}

int IncrementBuildCounter() {
    int currentCount = GetCurrentBuildCount();
    int newCount = currentCount + 1;
    
    std::ofstream file(BUILD_COUNTER_FILE);
    if (file.is_open()) {
        file << newCount;
        file.close();
        return newCount;
    }
    
    // If we can't write the file, return the current count
    // (build still attempted, but counter couldn't be updated)
    return currentCount;
}

int InitializeBuildCounter() {
    // Check if counter file already exists
    if (std::filesystem::exists(BUILD_COUNTER_FILE)) {
        return GetCurrentBuildCount();
    }
    
    // Create initial counter file
    std::ofstream file(BUILD_COUNTER_FILE);
    if (file.is_open()) {
        file << "1";  // Start with build #1
        file.close();
        return 1;
    }
    
    return 0;  // Fallback if file creation fails
}

std::string GetBuildCountString() {
    int count = GetCurrentBuildCount();
    if (count > 0) {
        std::ostringstream oss;
        oss << "Build #" << count;
        return oss.str();
    }
    return "Build #0";
}

} // namespace BuildCounter
} // namespace FluidNC
