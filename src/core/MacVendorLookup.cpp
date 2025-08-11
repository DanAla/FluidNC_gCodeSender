/**
 * core/MacVendorLookup.cpp
 * MAC address vendor lookup implementation
 */

#include "MacVendorLookup.h"

// Static member initialization
std::unordered_map<std::string, std::string> MacVendorLookup::s_ouiDatabase;
bool MacVendorLookup::s_initialized = false;

// Comprehensive OUI database focused on embedded systems, IoT, and network devices
const std::unordered_map<std::string, std::string> OUI_DATABASE = {
    // ESP32/ESP8266 - Very common for FluidNC and IoT projects
    {"30AEA4", "Espressif Systems (ESP32)"},
    {"240AC4", "Espressif Systems (ESP32/ESP8266)"},
    {"78421C", "Espressif Systems (ESP32/ESP8266)"},
    {"807D3A", "Espressif Systems (ESP32/ESP8266)"},
    {"84CCA8", "Espressif Systems (ESP32/ESP8266)"},
    {"8CAAB5", "Espressif Systems (ESP32/ESP8266)"},
    {"A020A6", "Espressif Systems (ESP32/ESP8266)"},
    {"CC50E3", "Espressif Systems (ESP32/ESP8266)"},
    {"DC4F22", "Espressif Systems (ESP32/ESP8266)"},
    {"E89F6D", "Espressif Systems (ESP32/ESP8266)"},
    {"F0B479", "Espressif Systems (ESP32/ESP8266)"},
    {"F4CFA2", "Espressif Systems (ESP32/ESP8266)"},
    {"FC1DDA", "Espressif Systems (ESP32/ESP8266)"},
    
    // Arduino and development boards
    {"2CF32F", "Arduino LLC"},
    {"A8A195", "Arduino"},
    {"90A2DA", "Arduino"},
    
    // Raspberry Pi
    {"B827EB", "Raspberry Pi Foundation"},
    {"DCA632", "Raspberry Pi Foundation"},
    {"E45F01", "Raspberry Pi Foundation"},
    {"DC21B2", "Raspberry Pi Trading"},
    {"28CD2E", "Raspberry Pi Trading"},
    
    // Common router/gateway manufacturers
    {"4CC64C", "NETGEAR"},
    {"A040A0", "NETGEAR"},
    {"E091F5", "NETGEAR"},
    {"2C30BD", "NETGEAR"},
    {"9C3DCF", "NETGEAR"},
    {"E0469A", "NETGEAR"},
    
    {"00259D", "TP-LINK"},
    {"001279", "TP-LINK"},
    {"14CC20", "TP-LINK"},
    {"1C61B4", "TP-LINK"},
    {"50C7BF", "TP-LINK"},
    {"A42BB0", "TP-LINK"},
    {"D8EB97", "TP-LINK"},
    {"F46D04", "TP-LINK"},
    
    {"001DD9", "ASUS"},
    {"001FC6", "ASUS"},
    {"00261E", "ASUS"},
    {"B06EBF", "ASUS"},
    {"F832E4", "ASUS"},
    {"1C872C", "ASUS"},
    {"2C4D54", "ASUS"},
    {"38D547", "ASUS"},
    {"50465D", "ASUS"},
    
    {"000C42", "Linksys"},
    {"000EA6", "Linksys"},
    {"001217", "Linksys"},
    {"0013C4", "Linksys"},
    {"001839", "Linksys"},
    {"001CDF", "Linksys"},
    {"0020F7", "Linksys"},
    {"48F8B3", "Linksys"},
    
    // D-Link
    {"001195", "D-Link"},
    {"0015E9", "D-Link"},
    {"001CF0", "D-Link"},
    {"14D64D", "D-Link"},
    {"90F652", "D-Link"},
    {"C8BE19", "D-Link"},
    
    // Apple devices
    {"001122", "Apple"},
    {"00A040", "Apple"},
    {"040CCE", "Apple"},
    {"0C4DE9", "Apple"},
    {"14109F", "Apple"},
    {"20C9D0", "Apple"},
    {"286AB8", "Apple"},
    {"2CAB25", "Apple"},
    {"50ED3C", "Apple"},
    
    // Samsung
    {"002454", "Samsung Electronics"},
    {"0025D3", "Samsung Electronics"},
    {"1C62B8", "Samsung Electronics"},
    {"2C598A", "Samsung Electronics"},
    {"70F395", "Samsung Electronics"},
    {"C85B76", "Samsung Electronics"},
    
    // Intel wireless cards
    {"001B77", "Intel"},
    {"0013CE", "Intel"},
    {"0024D7", "Intel"},
    {"1C659D", "Intel"},
    {"34F39A", "Intel"},
    {"7085C2", "Intel"},
    {"00D0B7", "Intel"},
    
    // Broadcom
    {"0010F3", "Broadcom"},
    {"001018", "Broadcom"},
    {"0014A4", "Broadcom"},
    
    // Realtek - common in cheap network equipment
    {"00E04C", "Realtek"},
    {"001CC0", "Realtek"},
    {"B0487A", "Realtek"},
    {"105BA9", "Realtek"},
    {"2C56DC", "Realtek"},
    
    // Other embedded/IoT manufacturers
    {"001EC0", "Texas Instruments"},
    {"0018B9", "Texas Instruments"},
    {"18FE34", "Texas Instruments"},
    {"70B3D5", "Texas Instruments"},
    
    {"00045A", "Microchip Technology"},
    {"001BC5", "Microchip Technology"},
    {"0004A3", "Microchip Technology"},
    
    {"00037A", "Nordic Semiconductor"},
    {"F01DB0", "Nordic Semiconductor"},
    
    {"F8633C", "Qualcomm"},
    {"009033", "Qualcomm"},
    {"38F23E", "Qualcomm"},
    
    // Common Chinese manufacturers
    {"001A11", "Shenzhen"},
    {"68DFDD", "Shenzhen"},
    {"E4956E", "Shenzhen"},
    
    // VMware/Virtual machines (useful for development)
    {"005056", "VMware"},
    {"000569", "VMware"},
    {"000C29", "VMware"},
    
    // Microsoft (Surface, Xbox, etc.)
    {"001DD8", "Microsoft"},
    {"0017FA", "Microsoft"},
    {"009027", "Microsoft"},
    {"7C1E52", "Microsoft"},
    
    // Generic/common patterns for broadcast
    {"FFFFFF", "Broadcast"},
    {"000000", "Invalid"},
};

std::string MacVendorLookup::GetVendor(const std::string& macAddress) {
    if (!s_initialized) {
        InitializeOuiDatabase();
        s_initialized = true;
    }
    
    std::string normalizedMac = NormalizeMac(macAddress);
    if (normalizedMac.length() < 6) {
        return "Unknown";
    }
    
    // Extract OUI (first 6 hex characters)
    std::string oui = normalizedMac.substr(0, 6);
    
    // First try local database
    auto it = s_ouiDatabase.find(oui);
    if (it != s_ouiDatabase.end()) {
        return it->second;
    }
    
    // Fallback to online lookup if not found locally
    std::string onlineResult = GetVendorOnline(macAddress);
    if (onlineResult != "Unknown" && !onlineResult.empty()) {
        // Cache the result in our local database for future use
        s_ouiDatabase[oui] = onlineResult;
        return onlineResult;
    }
    
    return "Unknown";
}

std::string MacVendorLookup::GetDeviceType(const std::string& macAddress, const std::string& vendor) {
    // Enhanced device type detection based on vendor
    std::string lowerVendor = vendor;
    std::transform(lowerVendor.begin(), lowerVendor.end(), lowerVendor.begin(), ::tolower);
    
    if (lowerVendor.find("espressif") != std::string::npos) {
        return "ESP32/ESP8266";
    }
    else if (lowerVendor.find("arduino") != std::string::npos) {
        return "Arduino";
    }
    else if (lowerVendor.find("raspberry") != std::string::npos) {
        return "Raspberry Pi";
    }
    else if (lowerVendor.find("netgear") != std::string::npos ||
             lowerVendor.find("tp-link") != std::string::npos ||
             lowerVendor.find("asus") != std::string::npos ||
             lowerVendor.find("linksys") != std::string::npos ||
             lowerVendor.find("d-link") != std::string::npos) {
        return "Router";
    }
    else if (lowerVendor.find("apple") != std::string::npos) {
        return "Apple Device";
    }
    else if (lowerVendor.find("samsung") != std::string::npos) {
        return "Samsung Device";
    }
    else if (lowerVendor.find("intel") != std::string::npos ||
             lowerVendor.find("broadcom") != std::string::npos ||
             lowerVendor.find("realtek") != std::string::npos) {
        return "Network Card";
    }
    else if (lowerVendor.find("vmware") != std::string::npos) {
        return "Virtual Machine";
    }
    else if (lowerVendor.find("texas instruments") != std::string::npos ||
             lowerVendor.find("microchip") != std::string::npos ||
             lowerVendor.find("nordic") != std::string::npos ||
             lowerVendor.find("qualcomm") != std::string::npos) {
        return "IoT Device";
    }
    
    return "Unknown";
}

std::string MacVendorLookup::NormalizeMac(const std::string& macAddress) {
    std::string normalized;
    normalized.reserve(12);
    
    for (char c : macAddress) {
        if (std::isxdigit(c)) {
            normalized += std::toupper(c);
        }
    }
    
    return normalized;
}

std::string MacVendorLookup::GetVendorOnline(const std::string& macAddress) {
    std::string normalizedMac = NormalizeMac(macAddress);
    if (normalizedMac.length() < 6) {
        return "Unknown";
    }
    
    // Extract the first 6 characters (OUI) and format as XX:XX:XX for the API
    std::string oui = normalizedMac.substr(0, 6);
    wxString formattedOui = wxString::Format("%c%c:%c%c:%c%c", 
                                           oui[0], oui[1], oui[2], 
                                           oui[3], oui[4], oui[5]);
    
    // Try multiple online APIs in order of preference
    std::string result;
    
    // 1. Try macvendors.com API (free, no rate limiting for reasonable use)
    result = QueryMacVendorsAPI(formattedOui);
    if (!result.empty() && result != "Unknown") {
        return result;
    }
    
    // 2. Try macvendors.co API as fallback
    result = QueryMacVendorsCoAPI(formattedOui);
    if (!result.empty() && result != "Unknown") {
        return result;
    }
    
    return "Unknown";
}

std::string MacVendorLookup::QueryMacVendorsAPI(const wxString& macAddress) {
    try {
        wxHTTP http;
        http.SetTimeout(3); // Short timeout to avoid blocking UI
        
        if (!http.Connect("macvendors.com")) {
            return "Unknown";
        }
        
        wxString path = "/query/" + macAddress;
        wxInputStream* stream = http.GetInputStream(path);
        if (!stream) {
            return "Unknown";
        }
        
        wxString response;
        char buffer[512];
        while (!stream->Eof()) {
            stream->Read(buffer, sizeof(buffer) - 1);
            size_t bytesRead = stream->LastRead();
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                response += wxString(buffer, wxConvUTF8);
            }
        }
        
        delete stream;
        
        // Clean up the response (remove newlines, extra whitespace)
        response.Trim(true).Trim(false);
        
        // If response is empty or contains error messages, return Unknown
        if (response.IsEmpty() || 
            response.Lower().Contains("not found") || 
            response.Lower().Contains("error")) {
            return "Unknown";
        }
        
        return std::string(response.mb_str(wxConvUTF8));
    }
    catch (...) {
        return "Unknown";
    }
}

std::string MacVendorLookup::QueryMacVendorsCoAPI(const wxString& macAddress) {
    try {
        wxHTTP http;
        http.SetTimeout(3); // Short timeout to avoid blocking UI
        
        if (!http.Connect("macvendors.co")) {
            return "Unknown";
        }
        
        wxString path = "/api/vendorname/" + macAddress;
        wxInputStream* stream = http.GetInputStream(path);
        if (!stream) {
            return "Unknown";
        }
        
        wxString response;
        char buffer[512];
        while (!stream->Eof()) {
            stream->Read(buffer, sizeof(buffer) - 1);
            size_t bytesRead = stream->LastRead();
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                response += wxString(buffer, wxConvUTF8);
            }
        }
        
        delete stream;
        
        // Clean up the response
        response.Trim(true).Trim(false);
        
        if (response.IsEmpty() || 
            response.Lower().Contains("not found") || 
            response.Lower().Contains("error")) {
            return "Unknown";
        }
        
        return std::string(response.mb_str(wxConvUTF8));
    }
    catch (...) {
        return "Unknown";
    }
}

void MacVendorLookup::InitializeOuiDatabase() {
    s_ouiDatabase = OUI_DATABASE;
}
