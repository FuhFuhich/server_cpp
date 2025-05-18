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
	// Любой тип, любое кол-во аргументов
	template <typename... Args>
	void log(fmt::format_string<Args...> fmt_str, Args&&... args)
	{
		std::lock_guard<std::mutex> lock(log_mutex_); // Работает только 1 поток
		log_file_ << "SERVER " << fmt::format(fmt_str, std::forward<Args>(args)...) << "\n";
		log_file_.flush(); // Очищаем буффер
	}
};
