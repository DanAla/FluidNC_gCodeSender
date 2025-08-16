#pragma once

#include "StringUtils.h"
#include <string>
#include <wx/string.h>

// Constants with guaranteed ASCII-only content
namespace Strings {
    // Status messages
    const wxString STATUS_CONNECTED = TO_WX("Connected");
    const wxString STATUS_DISCONNECTED = TO_WX("Disconnected");
    const wxString STATUS_ERROR = TO_WX("Error");
    const wxString STATUS_BUSY = TO_WX("Busy");
    const wxString STATUS_IDLE = TO_WX("Idle");
    const wxString STATUS_HOLD = TO_WX("Hold");
    const wxString STATUS_ALARM = TO_WX("Alarm");

    // Button labels
    const wxString BTN_CONNECT = TO_WX("Connect");
    const wxString BTN_DISCONNECT = TO_WX("Disconnect");
    const wxString BTN_CANCEL = TO_WX("Cancel");
    const wxString BTN_OK = TO_WX("OK");
    const wxString BTN_APPLY = TO_WX("Apply");
    const wxString BTN_SAVE = TO_WX("Save");
    const wxString BTN_LOAD = TO_WX("Load");
    const wxString BTN_RESET = TO_WX("Reset");

    // Common labels
    const wxString LABEL_PORT = TO_WX("Port");
    const wxString LABEL_BAUD = TO_WX("Baud Rate");
    const wxString LABEL_IP = TO_WX("IP Address");
    const wxString LABEL_MACHINE = TO_WX("Machine");
    const wxString LABEL_STATUS = TO_WX("Status");
    const wxString LABEL_POSITION = TO_WX("Position");
    const wxString LABEL_SETTINGS = TO_WX("Settings");

    // Error messages (ASCII-safe)
    const wxString ERROR_CONNECTION_FAILED = TO_WX("Connection failed");
    const wxString ERROR_INVALID_SETTINGS = TO_WX("Invalid settings");
    const wxString ERROR_FILE_NOT_FOUND = TO_WX("File not found");
    const wxString ERROR_OPERATION_FAILED = TO_WX("Operation failed");

    // Success messages
    const wxString SUCCESS_SAVED = TO_WX("Settings saved successfully");
    const wxString SUCCESS_LOADED = TO_WX("Settings loaded successfully");
    const wxString SUCCESS_CONNECTED = TO_WX("Connected successfully");
    const wxString SUCCESS_DISCONNECTED = TO_WX("Disconnected successfully");

    // UI element titles
    const wxString TITLE_MAIN_WINDOW = TO_WX("FluidNC gCode Sender");
    const wxString TITLE_SETTINGS = TO_WX("Settings");
    const wxString TITLE_CONNECTION = TO_WX("Connection Settings");
    const wxString TITLE_ERROR = TO_WX("Error");
    const wxString TITLE_WARNING = TO_WX("Warning");
    const wxString TITLE_SUCCESS = TO_WX("Success");

    // Menu labels
    const wxString MENU_FILE = TO_WX("File");
    const wxString MENU_EDIT = TO_WX("Edit");
    const wxString MENU_VIEW = TO_WX("View");
    const wxString MENU_TOOLS = TO_WX("Tools");
    const wxString MENU_HELP = TO_WX("Help");

    // File filters
    const wxString FILTER_GCODE = TO_WX("G-code files (*.nc;*.gcode)|*.nc;*.gcode|All files (*.*)|*.*");
    const wxString FILTER_CONFIG = TO_WX("Configuration files (*.json)|*.json|All files (*.*)|*.*");
} // namespace Strings
