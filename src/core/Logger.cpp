/**
 * core/Logger.cpp
 * Implementation of comprehensive logging system
 */

#include "Logger.h"
#include <filesystem>
#include <iomanip>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    shutdown();
}

void Logger::initialize(const std::string& logFilePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        return; // Already initialized
    }
    
    // Create logs directory if it doesn't exist
    std::filesystem::path logsDir = "logs";
    std::filesystem::create_directories(logsDir);
    
    // Determine log file path
    std::string actualLogPath;
    if (logFilePath.empty()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream oss;
        oss << "logs/FluidNC_gCodeSender_" 
            << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") 
            << ".log";
        actualLogPath = oss.str();
    } else {
        actualLogPath = logFilePath;
    }
    
    // Open log file
    if (m_fileOutput) {
        m_fileStream = std::make_unique<std::ofstream>(actualLogPath, std::ios::app);
        if (!m_fileStream->is_open()) {
            std::cerr << "Warning: Could not open log file: " << actualLogPath << std::endl;
            m_fileOutput = false;
        } else {
            *m_fileStream << "\n" << std::string(80, '=') << "\n";
            *m_fileStream << "FluidNC gCode Sender - Log Session Started\n";
            *m_fileStream << "Timestamp: " << getCurrentTimestamp() << "\n";
            *m_fileStream << "Log file: " << actualLogPath << "\n";
            *m_fileStream << std::string(80, '=') << "\n\n";
            m_fileStream->flush();
        }
    }
    
    m_initialized = true;
    
    // Log the initialization
    info("Logger initialized successfully", __FILE__, __LINE__, __FUNCTION__);
    info("Log file: " + actualLogPath, __FILE__, __LINE__, __FUNCTION__);
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    if (m_fileStream && m_fileStream->is_open()) {
        *m_fileStream << "\n" << std::string(80, '=') << "\n";
        *m_fileStream << "FluidNC gCode Sender - Log Session Ended\n";
        *m_fileStream << "Timestamp: " << getCurrentTimestamp() << "\n";
        *m_fileStream << std::string(80, '=') << "\n";
        m_fileStream->close();
    }
    
    m_initialized = false;
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logLevel = level;
}

void Logger::enableConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_consoleOutput = enable;
}

void Logger::enableFileOutput(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fileOutput = enable;
}

void Logger::log(LogLevel level, const std::string& message, 
                const std::string& file, int line, const std::string& function) {
    
    if (level < m_logLevel) {
        return; // Below log level threshold
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string formattedMessage = formatMessage(level, message, file, line, function);
    
    // Console output
    if (m_consoleOutput) {
        if (level >= LogLevel::ERR) {
            std::cerr << formattedMessage << std::endl;
        } else {
            std::cout << formattedMessage << std::endl;
        }
    }
    
    // File output
    if (m_fileOutput && m_fileStream && m_fileStream->is_open()) {
        *m_fileStream << formattedMessage << std::endl;
        m_fileStream->flush(); // Ensure immediate write for debugging
    }
}

void Logger::trace(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::TRACE, message, file, line, function);
}

void Logger::debug(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::DEBUG, message, file, line, function);
}

void Logger::info(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::INFO, message, file, line, function);
}

void Logger::warn(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::WARN, message, file, line, function);
}

void Logger::error(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::ERR, message, file, line, function);
}

void Logger::fatal(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::FATAL, message, file, line, function);
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Logger::formatMessage(LogLevel level, const std::string& message, 
                                 const std::string& file, int line, const std::string& function) {
    std::ostringstream oss;
    
    oss << "[" << getCurrentTimestamp() << "] ";
    oss << "[" << levelToString(level) << "] ";
    
    if (!file.empty() && line > 0) {
        // Extract just the filename from the full path
        std::filesystem::path filePath(file);
        oss << "[" << filePath.filename().string() << ":" << line;
        if (!function.empty()) {
            oss << " in " << function << "()";
        }
        oss << "] ";
    }
    
    oss << message;
    
    return oss.str();
}
