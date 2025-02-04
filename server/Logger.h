#pragma once

#include <mutex>
#include <fstream>
#include <fmt/core.h>

class Logger
{
private:
	std::ofstream log_file_;
	std::mutex log_mutex_;

public:
	Logger() = delete;
	Logger(const std::string& filename);
	~Logger();
	template <typename... Args>
	void log(fmt::format_string<Args...> fmt_str, Args&&... args);
};
