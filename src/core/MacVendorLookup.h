/**
 * core/MacVendorLookup.h
 * MAC address vendor lookup using OUI (Organizationally Unique Identifier) database
 * Focus on embedded systems, IoT devices, and common network equipment
 */

#pragma once

#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <wx/wx.h>
#include <wx/url.h>
#include <wx/protocol/http.h>
#include <wx/stream.h>

class MacVendorLookup {
public:
    static std::string GetVendor(const std::string& macAddress);
    static std::string GetVendorOnline(const std::string& macAddress);
    static std::string GetDeviceType(const std::string& macAddress, const std::string& vendor);
    
private:
    static std::string NormalizeMac(const std::string& macAddress);
    static void InitializeOuiDatabase();
    static std::string QueryMacVendorsAPI(const wxString& macAddress);
    static std::string QueryMacVendorsCoAPI(const wxString& macAddress);
    
    static std::unordered_map<std::string, std::string> s_ouiDatabase;
    static bool s_initialized;
};

// Common OUI prefixes for embedded systems and IoT devices
// Format: "AABBCC" -> "Vendor Name"
extern const std::unordered_map<std::string, std::string> OUI_DATABASE;
