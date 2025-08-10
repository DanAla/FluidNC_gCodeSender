/**
 * core/Version.h
 * Version information and build details for FluidNC gCode Sender
 */

#pragma once

#include <string>

namespace FluidNC {
namespace Version {

// Version information (populated by CMake from Git tags)
constexpr const char* APP_NAME = "FluidNC gCode Sender";

// Use CMake-provided version information, with fallbacks
#ifndef VERSION_MAJOR
#define VERSION_MAJOR "0"
#endif
#ifndef VERSION_MINOR
#define VERSION_MINOR "1"
#endif
#ifndef VERSION_PATCH
#define VERSION_PATCH "0"
#endif
#ifndef VERSION_STRING
#define VERSION_STRING "0.1.0"
#endif

constexpr const char* VERSION_MAJOR_STR = VERSION_MAJOR;
constexpr const char* VERSION_MINOR_STR = VERSION_MINOR;
constexpr const char* VERSION_PATCH_STR = VERSION_PATCH;
constexpr const char* VERSION_STRING_STR = VERSION_STRING;

// Repository information
constexpr const char* REPOSITORY_URL = "https://github.com/DanAla/FluidNC_gCodeSender";
constexpr const char* ISSUES_URL = "https://github.com/DanAla/FluidNC_gCodeSender/issues";

// Build information (to be populated by CMake)
#ifndef BUILD_TIMESTAMP
#define BUILD_TIMESTAMP __DATE__ " " __TIME__
#endif

#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH "unknown"
#endif

#ifndef GIT_BRANCH
#define GIT_BRANCH "main"
#endif

#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
#endif

#ifndef BUILD_TYPE
#ifdef _DEBUG
#define BUILD_TYPE "Debug"
#else
#define BUILD_TYPE "Release"
#endif
#endif

constexpr const char* BUILD_INFO = BUILD_TIMESTAMP;
constexpr const char* COMMIT_HASH = GIT_COMMIT_HASH;
constexpr const char* BRANCH = GIT_BRANCH;
constexpr const char* DESCRIBE = GIT_DESCRIBE;
constexpr const char* BUILD_CONFIG = BUILD_TYPE;

// Platform information
#ifdef _WIN32
constexpr const char* PLATFORM = "Windows";
#elif defined(__linux__)
constexpr const char* PLATFORM = "Linux";
#elif defined(__APPLE__)
constexpr const char* PLATFORM = "macOS";
#else
constexpr const char* PLATFORM = "Unknown";
#endif

#ifdef _WIN64
constexpr const char* ARCHITECTURE = "x64";
#elif defined(_WIN32)
constexpr const char* ARCHITECTURE = "x86";
#else
constexpr const char* ARCHITECTURE = "unknown";
#endif

// Compiler information
#ifdef __GNUC__
constexpr const char* COMPILER = "GCC";
#elif defined(_MSC_VER)
constexpr const char* COMPILER = "MSVC";
#elif defined(__clang__)
constexpr const char* COMPILER = "Clang";
#else
constexpr const char* COMPILER = "Unknown";
#endif

// Helper functions
std::string GetFullVersionString();
std::string GetBuildInfoString();
std::string GetAboutInfoString();

} // namespace Version
} // namespace FluidNC
