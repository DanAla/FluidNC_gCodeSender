/**
 * core/SimpleLogger.h
 * Simple, basic logging system that actually works
 */

#pragma once

#include <string>
#include <fstream>
#include <iostream>

/**
 * Simple logging class - no fancy features, just works!
 */
class SimpleLogger {
public:
    // Get singleton instance
    static SimpleLogger& getInstance();
    
    // Simple logging methods
    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
    void logDebug(const std::string& message);
    
private:
    SimpleLogger() = default;
    ~SimpleLogger();
    
    void ensureLogFile();
    std::string getTimestamp();
    
    std::ofstream m_logFile;
    bool m_logFileOpen = false;
};

// Simple macros
#define LOG_INFO(msg) SimpleLogger::getInstance().logInfo(msg)
#define LOG_WARNING(msg) SimpleLogger::getInstance().logWarning(msg)
#define LOG_ERROR(msg) SimpleLogger::getInstance().logError(msg)
#define LOG_DEBUG(msg) SimpleLogger::getInstance().logDebug(msg)
