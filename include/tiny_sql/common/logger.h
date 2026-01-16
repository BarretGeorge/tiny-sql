#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <mutex>

namespace tiny_sql {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void setLevel(LogLevel level) {
        level_ = level;
    }

    LogLevel getLevel() const {
        return level_;
    }

    void log(LogLevel level, const std::string& file, int line,
             const std::string& message) {
        if (level < level_) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        std::ostringstream oss;

        // 时间戳
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

        // 日志级别
        oss << " [" << levelToString(level) << "] ";

        // 文件和行号
        oss << "[" << file << ":" << line << "] ";

        // 消息
        oss << message << std::endl;

        std::cout << oss.str();
        std::cout.flush();
    }

private:
    Logger() : level_(LogLevel::INFO) {}

    std::string levelToString(LogLevel level) const {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO ";
            case LogLevel::WARN:  return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    LogLevel level_;
    std::mutex mutex_;
};

// 便捷宏
#define LOG_DEBUG(msg) \
    do { \
        std::ostringstream oss; \
        oss << msg; \
        tiny_sql::Logger::instance().log(tiny_sql::LogLevel::DEBUG, __FILE__, __LINE__, oss.str()); \
    } while (0)

#define LOG_INFO(msg) \
    do { \
        std::ostringstream oss; \
        oss << msg; \
        tiny_sql::Logger::instance().log(tiny_sql::LogLevel::INFO, __FILE__, __LINE__, oss.str()); \
    } while (0)

#define LOG_WARN(msg) \
    do { \
        std::ostringstream oss; \
        oss << msg; \
        tiny_sql::Logger::instance().log(tiny_sql::LogLevel::WARN, __FILE__, __LINE__, oss.str()); \
    } while (0)

#define LOG_ERROR(msg) \
    do { \
        std::ostringstream oss; \
        oss << msg; \
        tiny_sql::Logger::instance().log(tiny_sql::LogLevel::ERROR, __FILE__, __LINE__, oss.str()); \
    } while (0)

#define LOG_FATAL(msg) \
    do { \
        std::ostringstream oss; \
        oss << msg; \
        tiny_sql::Logger::instance().log(tiny_sql::LogLevel::FATAL, __FILE__, __LINE__, oss.str()); \
    } while (0)

} // namespace tiny_sql
