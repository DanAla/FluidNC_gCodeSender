#pragma once

#include "StringUtils.h"
#include <string>
#include <wx/string.h>
#include <wx/filename.h>
#include <filesystem>

namespace PathUtils {

/**
 * Ensures a file path contains only ASCII characters and is valid.
 * Any non-ASCII characters are replaced with underscores.
 */
inline std::string sanitizeFilePath(const std::string& path) {
    std::string result = path;
    std::replace_if(result.begin(), result.end(),
        [](unsigned char c) { return c > 127; }, '_');
    return result;
}

/**
 * Converts a wxString path to an ASCII-safe std::string path.
 */
inline std::string toASCIIPath(const wxString& path) {
    return sanitizeFilePath(TO_ASCII(path));
}

/**
 * Creates a wxString from an ASCII path string.
 */
inline wxString toWxPath(const std::string& path) {
    return TO_WX(sanitizeFilePath(path));
}

/**
 * Get the filename component of a path, ensuring ASCII-safety.
 */
inline std::string getASCIIFilename(const std::string& path) {
    wxFileName fn(TO_WX(path));
    return TO_ASCII(fn.GetFullName());
}

/**
 * Join path components together, ensuring ASCII-safety.
 */
inline std::string joinPaths(const std::string& base, const std::string& relative) {
    std::filesystem::path basePath(sanitizeFilePath(base));
    std::filesystem::path relativePath(sanitizeFilePath(relative));
    return (basePath / relativePath).string();
}

} // namespace PathUtils

// Helper macros for quick conversion
#define TO_ASCII_PATH(wx_path) PathUtils::toASCIIPath(wx_path)
#define TO_WX_PATH(std_path) PathUtils::toWxPath(std_path)
