#pragma once

#include <wx/string.h>
#include <string>
#include <cctype>
#include <algorithm>

namespace StringUtils {

/**
 * Ensures a string contains only ASCII characters (0-127).
 * Any non-ASCII characters are replaced with '?'.
 */
inline std::string enforceASCII(const std::string& input) {
    std::string result = input;
    std::replace_if(result.begin(), result.end(),
        [](unsigned char c) { return c > 127; }, '?');
    return result;
}

/**
 * Converts a wxString to ASCII-only std::string.
 * Any non-ASCII characters are replaced with '?'.
 */
inline std::string toASCIIString(const wxString& input) {
    return enforceASCII(input.ToStdString());
}

/**
 * Creates a wxString from an ASCII-only std::string.
 * This is safe because ASCII is a subset of Unicode.
 */
inline wxString toWxString(const std::string& input) {
    return wxString::FromUTF8(enforceASCII(input));
}

} // namespace StringUtils

// Helper macros for quick conversion
#define TO_ASCII(wx_str) StringUtils::toASCIIString(wx_str)
#define TO_WX(std_str) StringUtils::toWxString(std_str)
