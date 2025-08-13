/**
 * core/Version.h
 * Version information and build details for FluidNC gCode Sender
 */

#pragma once

#include <string>

// Include build-time generated version information
#include "generated_version.h"

namespace FluidNC {
namespace Version {

// Version information (populated at build time from Git tags)
constexpr const char* APP_NAME = "FluidNC gCode Sender";

// Version constants are now defined in generated_version.h
// The defines from generated_version.h are used directly in code
// VERSION_STRING_STR is available as a macro from generated_version.h

// Repository information
constexpr const char* REPOSITORY_URL = "https://github.com/DanAla/FluidNC_gCodeSender";
constexpr const char* ISSUES_URL = "https://github.com/DanAla/FluidNC_gCodeSender/issues";

// Build information (REQUIRED - populated by CMake)
#ifndef BUILD_TIMESTAMP
#error "BUILD_TIMESTAMP not defined! CMake build info extraction failed."
#endif

#ifndef GIT_COMMIT_HASH
#error "GIT_COMMIT_HASH not defined! CMake Git extraction failed."
#endif

#ifndef GIT_BRANCH
#error "GIT_BRANCH not defined! CMake Git extraction failed."
#endif

#ifndef GIT_DESCRIBE
#error "GIT_DESCRIBE not defined! CMake Git extraction failed."
#endif

// GIT_TAG is now defined in generated_version.h

#ifndef BUILD_TYPE
#ifdef _DEBUG
#define BUILD_TYPE "Debug"
#else
#define BUILD_TYPE "Release"
#endif
#endif

// Dependency versions (REQUIRED - populated by CMake)
#ifndef COMPILER_VERSION
#error "COMPILER_VERSION not defined! CMake compiler detection failed."
#endif

#ifndef CMAKE_VERSION_STRING
#error "CMAKE_VERSION_STRING not defined! CMake version detection failed."
#endif

#ifndef WXWIDGETS_VERSION
#error "WXWIDGETS_VERSION not defined! CMake wxWidgets detection failed."
#endif

#ifndef NLOHMANN_JSON_VERSION
#error "NLOHMANN_JSON_VERSION not defined! CMake JSON detection failed."
#endif

#ifndef MINGW_TARGET
#error "MINGW_TARGET not defined! CMake MinGW detection failed."
#endif

constexpr const char* BUILD_INFO = BUILD_TIMESTAMP;
constexpr const char* COMMIT_HASH = GIT_COMMIT_HASH;
constexpr const char* BRANCH = GIT_BRANCH;
constexpr const char* DESCRIBE = GIT_DESCRIBE;
constexpr const char* BUILD_CONFIG = BUILD_TYPE;
constexpr const char* GCC_VERSION = COMPILER_VERSION;
constexpr const char* CMAKE_VER = CMAKE_VERSION_STRING;
constexpr const char* WXWIDGETS_VER = WXWIDGETS_VERSION;
constexpr const char* JSON_VER = NLOHMANN_JSON_VERSION;
constexpr const char* MINGW_ARCH = MINGW_TARGET;

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
std::string GetFeaturesString();

} // namespace Version
} // namespace FluidNC
