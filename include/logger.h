#pragma once

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <utility>

namespace enc_logger {

// TODO: To be moved to a more appropriate location
inline const std::filesystem::path DEFAULT_LOG_FILE_PATH{
    "C:/Users/Vishal/Documents/workspace/encrypter/build/run_log/log.log"};

class Logger {
public:
    enum class Level {
        Error,
        Warning,
        Info,
        Debug,
    };

    static Logger& get_logger() {
        static Logger logger_instance(determine_log_level());
        return logger_instance;
    }

    template <typename... Args> void debug(std::format_string<Args...> fmt, Args&&... args) const {
        log(Level::Debug, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args> void info(std::format_string<Args...> fmt, Args&&... args) const {
        log(Level::Info, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warning(std::format_string<Args...> fmt, Args&&... args) const {
        log(Level::Warning, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args> void error(std::format_string<Args...> fmt, Args&&... args) const {
        log(Level::Error, fmt, std::forward<Args>(args)...);
    }

    void set_console_log_enabled(const bool& enabled) {
        console_log_enabled_ = enabled;
        debug("Console logging is now enabled : {}", enabled);
    }

    void set_file_log_enabled(const std::filesystem::path& logfile_path) {
        /* TODO: Remove path check from parent if,
         * we're disabling file logging by default for now when path isn't passed
         * Change the logic flow accordingly later
         */
        if (!logfile_path.empty() && !log_file_stream_) {
            check_log_file_path(logfile_path);
            if (logfile_path.empty()) {
                log_file_stream_ =
                    std::make_unique<std::fstream>(DEFAULT_LOG_FILE_PATH.string(), std::ios::out);
                debug("No file path provided, using default: {}", DEFAULT_LOG_FILE_PATH.string());
            } else {
                log_file_stream_ =
                    std::make_unique<std::fstream>(logfile_path.string(), std::ios::out);
                debug("File path passed as part of arguments: {}", logfile_path.string());
            }
        }
        debug("File logging is now enabled : {}", log_file_stream_ && log_file_stream_->is_open());
    }

private:
    Level log_level_;
    bool console_log_enabled_;
    std::unique_ptr<std::fstream> log_file_stream_;

    // TODO: Default console logging is enabled and file is disabled, After GUI integration switch
    // this
    explicit Logger(Level log_level, bool file_log_enabled = false, bool console_log_enabled = true)
        : log_level_(log_level), console_log_enabled_(console_log_enabled) {
        std::cout << std::format("Logger initialized with log level: {}", to_string(log_level_))
                  << std::endl;
    }

    /*
     * TODO: Maybe create a separate utility function for this later to check directory
     */
    void check_log_file_path(const std::filesystem::path& logfile_path) {
        if (std::filesystem::is_directory(logfile_path.parent_path())) {
            std::cout << std::format("Directory Exists : {}", logfile_path.parent_path().string())
                      << std::endl;
        } else {
            std::cout << std::format("Directory Does Not Exist, Creating directory: {}",
                                     logfile_path.parent_path().string())
                      << std::endl;
            std::filesystem::create_directories(logfile_path.parent_path().string());
        }
    }

    static Level determine_log_level() {
        const char* log_level_env = std::getenv("APP_LOG_LEVEL");
        if (log_level_env == nullptr) {
            return Level::Info;
        }

        std::string_view app_log_level{log_level_env};
        std::cout << "Log Level from ENV: " << app_log_level << std::endl;
        std::println("Log Level from Env: {}", app_log_level);

        if (app_log_level == to_string(Level::Debug)) {
            return Level::Debug;
        } else if (app_log_level == to_string(Level::Warning)) {
            return Level::Warning;
        } else {
            return Level::Info;
        }
    }

    template <typename... Args>
    void log(const Level& severity, const std::format_string<Args...>& fmt_str,
             Args&&... args) const {
        if (severity > log_level_) {
            return;
        }

        std::string formatted_string = std::format("[{}] ", to_string(severity)) +
                                       std::format(fmt_str, std::forward<Args>(args)...);

        if (log_file_stream_ && log_file_stream_->good()) {
            *log_file_stream_ << formatted_string;
            *log_file_stream_ << '\n';
        }

        if (console_log_enabled_) {
            std::println("{}", formatted_string);
        }
    }

    static const char* to_string(Logger::Level log_level) {
        switch (log_level) {
            case Level::Debug:
                return "DEBUG";
            case Level::Info:
                return "INFO";
            case Level::Warning:
                return "WARNING";
            case Level::Error:
                return "ERROR";
            default:
                return "WARNING";
        }
    }
};

// Global logger instance - use this throughout the codebase
inline Logger& log = Logger::get_logger();

} // namespace enc_logger
