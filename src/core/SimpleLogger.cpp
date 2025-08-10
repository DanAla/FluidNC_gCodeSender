/**
 * core/SimpleLogger.cpp  
 * Simple, basic logging system implementation
 */

#include "SimpleLogger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

SimpleLogger& SimpleLogger::getInstance() {
    static SimpleLogger instance;
    return instance;
}

SimpleLogger::~SimpleLogger() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void SimpleLogger::ensureLogFile() {
    if (!m_logFileOpen) {
        // Create logs directory if it doesn't exist
        std::filesystem::create_directories("logs");
        
        // Open log file with current date/time
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream filename;
        filename << "logs/FluidNC_" 
                 << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") 
                 << ".log";
        
        m_logFile.open(filename.str(), std::ios::out | std::ios::app);
        if (m_logFile.is_open()) {
            m_logFileOpen = true;
            m_logFile << "=== FluidNC gCode Sender Log Started ===" << std::endl;
        }
    }
}

std::string SimpleLogger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void SimpleLogger::logInfo(const std::string& message) {
    ensureLogFile();
    
    std::string logEntry = "[" + getTimestamp() + "] [INFO] " + message;
    
    // Write to console
    std::cout << logEntry << std::endl;
    
    // Write to file
    if (m_logFile.is_open()) {
        m_logFile << logEntry << std::endl;
        m_logFile.flush();
    }
}

void SimpleLogger::logError(const std::string& message) {
    ensureLogFile();
    
    std::string logEntry = "[" + getTimestamp() + "] [ERROR] " + message;
    
    // Write to console (stderr)
    std::cerr << logEntry << std::endl;
    
    // Write to file
    if (m_logFile.is_open()) {
        m_logFile << logEntry << std::endl;
        m_logFile.flush();
    }
}

void SimpleLogger::logDebug(const std::string& message) {
    ensureLogFile();
    
    std::string logEntry = "[" + getTimestamp() + "] [DEBUG] " + message;
    
    // Write to console
    std::cout << logEntry << std::endl;
    
    // Write to file
    if (m_logFile.is_open()) {
        m_logFile << logEntry << std::endl;
        m_logFile.flush();
    }
}
