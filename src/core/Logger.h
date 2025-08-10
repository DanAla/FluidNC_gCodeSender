/**
 * core/Logger.h
 * Comprehensive logging system for debugging application startup and runtime issues
 */

#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <mutex>
#include <memory>

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERR = 4,    // Renamed to avoid conflict with Windows ERROR macro
    FATAL = 5
};

class Logger {
public:
    static Logger& getInstance();
    
    void initialize(const std::string& logFilePath = "");
    void shutdown();
    
    void setLogLevel(LogLevel level);
    void enableConsoleOutput(bool enable);
    void enableFileOutput(bool enable);
    
    void log(LogLevel level, const std::string& message, 
             const std::string& file = "", int line = 0, const std::string& function = "");
    
    // Convenience methods
    void trace(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "");
    void debug(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "");
    void info(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "");
    void warn(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "");
    void error(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "");
    void fatal(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "");

private:
    Logger() = default;
    ~Logger();
    
    std::string getCurrentTimestamp();
    std::string levelToString(LogLevel level);
    std::string formatMessage(LogLevel level, const std::string& message, 
                             const std::string& file, int line, const std::string& function);
    
    std::mutex m_mutex;
    std::unique_ptr<std::ofstream> m_fileStream;
    LogLevel m_logLevel = LogLevel::DEBUG;
    bool m_consoleOutput = true;
    bool m_fileOutput = true;
    bool m_initialized = false;
};

// Convenient macros for logging with automatic file/line/function info
#define LOG_TRACE(msg) Logger::getInstance().trace((msg), __FILE__, __LINE__, __FUNCTION__)
#define LOG_DEBUG(msg) Logger::getInstance().debug((msg), __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(msg) Logger::getInstance().info((msg), __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARN(msg) Logger::getInstance().warn((msg), __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(msg) Logger::getInstance().error((msg), __FILE__, __LINE__, __FUNCTION__)
#define LOG_FATAL(msg) Logger::getInstance().fatal((msg), __FILE__, __LINE__, __FUNCTION__)

// Stream-based logging macros for formatted messages
#define LOG_TRACE_STREAM(stream) do { \
    std::ostringstream oss; \
    oss << stream; \
    LOG_TRACE(oss.str()); \
} while(0)

#define LOG_DEBUG_STREAM(stream) do { \
    std::ostringstream oss; \
    oss << stream; \
    LOG_DEBUG(oss.str()); \
} while(0)

#define LOG_INFO_STREAM(stream) do { \
    std::ostringstream oss; \
    oss << stream; \
    LOG_INFO(oss.str()); \
} while(0)

#define LOG_WARN_STREAM(stream) do { \
    std::ostringstream oss; \
    oss << stream; \
    LOG_WARN(oss.str()); \
} while(0)

#define LOG_ERROR_STREAM(stream) do { \
    std::ostringstream oss; \
    oss << stream; \
    LOG_ERROR(oss.str()); \
} while(0)

#define LOG_FATAL_STREAM(stream) do { \
    std::ostringstream oss; \
    oss << stream; \
    LOG_FATAL(oss.str()); \
} while(0)
