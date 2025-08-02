// src/common/logger.h
#pragma once
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger {
public:
    enum Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    static void log(Level level, const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        std::stringstream ss;
        ss << std::put_time(&tm, "%H:%M:%S");
        
        std::string level_str;
        switch (level) {
            case DEBUG: level_str = "DEBUG"; break;
            case INFO: level_str = "INFO"; break;
            case WARNING: level_str = "WARN"; break;
            case ERROR: level_str = "ERROR"; break;
        }
        
        std::cout << "[" << ss.str() << "] [" << level_str << "] " << message << std::endl;
    }
    
    static void debug(const std::string& message) { log(DEBUG, message); }
    static void info(const std::string& message) { log(INFO, message); }
    static void warning(const std::string& message) { log(WARNING, message); }
    static void error(const std::string& message) { log(ERROR, message); }
};

#define LOG_DEBUG(msg) Logger::debug(msg)
#define LOG_INFO(msg) Logger::info(msg)
#define LOG_WARNING(msg) Logger::warning(msg)
#define LOG_ERROR(msg) Logger::error(msg)