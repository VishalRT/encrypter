#pragma once

#include <format>
#include <iostream>
#include <iterator>
#include <ostream>
#include <string_view>
#include <utility>

namespace logger {
class Logger {
public:
    enum class Level {
        Error,
        Warning,
        Info,
        Debug,
    };

    static Logger& get_logger();

    void info(std::string_view message) const;
    template <typename... Args> void info(std::format_string<Args...> fmt, Args&&... args) const {
        log(Level::Info, fmt, std::forward<Args>(args)...);
    }

    void warning(std::string_view message) const;
    template <typename... Args>
    void warning(std::format_string<Args...> fmt, Args&&... args) const {
        log(Level::Warning, fmt, std::forward<Args>(args)...);
    }

    void error(std::string_view message) const;
    template <typename... Args> void error(std::format_string<Args...> fmt, Args&&... args) const {
        log(Level::Error, fmt, std::forward<Args>(args)...);
    }

    void debug(std::string_view message) const;
    template <typename... Args> void debug(std::format_string<Args...> fmt, Args&&... args) const {
        log(Level::Debug, fmt, std::forward<Args>(args)...);
    }

private:
    Logger() = default;

    void log(Level severity, std::string_view message) const;

    template <typename... Args>
    void log(Level severity, std::format_string<Args...> fmt, Args&&... args) const {
        std::ostream& output_stream = severity == Level::Error ? std::cerr : std::cout;
        output_stream << '[' << level_to_string(severity) << "] ";
        std::format_to(std::ostreambuf_iterator<char>(output_stream), fmt,
                       std::forward<Args>(args)...);
        output_stream << '\n';
    }

    static const char* level_to_string(Level severity);
};

inline Logger& Logger::get_logger() {
    static Logger instance;
    return instance;
}

inline void Logger::info(std::string_view message) const {
    log(Level::Info, message);
}

inline void Logger::warning(std::string_view message) const {
    log(Level::Warning, message);
}

inline void Logger::error(std::string_view message) const {
    log(Level::Error, message);
}

inline void Logger::debug(std::string_view message) const {
    log(Level::Debug, message);
}

inline void Logger::log(Level severity, std::string_view message) const {
    std::ostream& output_stream = severity == Level::Error ? std::cerr : std::cout;
    output_stream << '[' << level_to_string(severity) << "] " << message << '\n';
}

inline const char* Logger::level_to_string(Level severity) {
    switch (severity) {
        case Level::Info:
            return "INFO";
        case Level::Warning:
            return "WARNING";
        case Level::Error:
            return "ERROR";
        case Level::Debug:
            return "DEBUG";
        default:
            return "INFO";
    }
}

// Global logger instance - use this throughout the codebase
inline Logger& log = Logger::get_logger();

} // namespace logger
