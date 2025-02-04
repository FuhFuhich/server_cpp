#pragma once

#include <mutex>
#include <fstream>
#include <fmt/core.h>
#include <utility>

class Logger
{
private:
	std::ofstream log_file_;
	std::mutex log_mutex_;

public:
	Logger() = delete;
	Logger(const std::string& filename);
	~Logger();

public:
	template <typename... Args>
	void log(fmt::format_string<Args...> fmt_str, Args&&... args)
	{
		std::lock_guard<std::mutex> lock(log_mutex_);
		log_file_ << "SERVER " << fmt::format(fmt_str, std::forward<Args>(args)...) << "\n";
	}
};
