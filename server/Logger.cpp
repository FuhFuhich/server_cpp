#include "Logger.h"

Logger::Logger(const std::string& filename)
{
	log_file_.open(filename, std::ios::app);
	log_file_.setf(std::ios::unitbuf);

	if (!log_file_.is_open())
	{
		throw std::runtime_error("Failed open log file in LOGGER");
	}
}

Logger::~Logger()
{
	if (log_file_.is_open()) 
	{
		log_file_ << "Close log_file\n";
		log_file_.close();
	}
}

void Logger::log(const std::string& message)
{
	std::lock_guard<std::mutex> lock(log_mutex_);
	log_file_ << message << "\n";
}