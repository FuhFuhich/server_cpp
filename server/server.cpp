#include "Lobby.h"

int main()
{
	try
	{
		setlocale(0, "");
		boost::asio::io_context io_context;
		short port = 5400;
		Lobby Lobby(io_context, port);
		io_context.run();
	}
	catch (const std::exception& e)
	{
		std::ofstream log_file_("server_output.txt", std::ios::app);
		log_file_.setf(std::ios::unitbuf);
		if (log_file_.is_open())
		{
			log_file_ << "Exception in MAIN: " << e.what() << "\n";
			log_file_.close();
		}
		else
		{
			throw std::runtime_error("Failed to open log file IN MAIN.");
		}
	}
}
