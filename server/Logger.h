#pragma once

#include <mutex>
#include <fstream>

class Logger
{
private:
	std::ofstream log_file_;
	std::mutex log_mutex_;

public:
	Logger() = delete;
	Logger(const std::string& filename);
	~Logger();
	void log(const std::string& message);
};
