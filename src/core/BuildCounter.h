/**
 * core/BuildCounter.h
 * Build counter management for tracking compilation attempts
 */

#pragma once

#include <string>

namespace FluidNC {
namespace BuildCounter {

// Build counter file path (relative to executable)
constexpr const char* BUILD_COUNTER_FILE = "build_counter.txt";

/**
 * Increment the build counter and return the new count
 * This should be called every time a build is attempted
 */
int IncrementBuildCounter();

/**
 * Get the current build counter value
 * Returns 0 if counter file doesn't exist
 */
int GetCurrentBuildCount();

/**
 * Initialize/create build counter file if it doesn't exist
 * Returns the initial count
 */
int InitializeBuildCounter();

/**
 * Get build counter as formatted string for display
 * Example: "Build #42"
 */
std::string GetBuildCountString();

} // namespace BuildCounter
} // namespace FluidNC
