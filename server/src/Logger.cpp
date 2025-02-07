#include "Logger.h"

Logger::Logger(const std::string& filename)
{
	log_file_.open(filename, std::ios::app);
	log_file_.setf(std::ios::unitbuf); // Убираем буфферизацию и сразу записываем

	if (!log_file_.is_open())
	{
		throw std::runtime_error("SERVER Failed open log file in LOGGER");
	}
}

Logger::~Logger()
{
	if (log_file_.is_open()) 
	{
		log_file_ << "SERVER Close log_file\n";
		log_file_.close();
	}
}
