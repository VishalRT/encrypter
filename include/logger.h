#pragma once

#include <format>
#include <iostream>
#include <iterator>
#include <ostream>
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

	static Logger& get_logger() {
		static Logger logger_instance(Level::Info);
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

private:
	Level log_level_;

	explicit Logger(Level log_level) : log_level_(log_level) {
	}

	template <typename... Args>
	void log(Level severity, std::format_string<Args...> fmt, Args&&... args) const {
		if (severity > log_level_)
			return;
		std::ostream& output_stream = severity == Level::Error ? std::cerr : std::cout;
		output_stream << '[' << to_string(severity) << "] ";
		std::format_to(std::ostreambuf_iterator<char>(output_stream), fmt,
					   std::forward<Args>(args)...);
		output_stream << '\n';
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

} // namespace logger
