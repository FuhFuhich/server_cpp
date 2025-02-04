#include "Lobby.h"
#include "logger.h"

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
		std::string filename = "server_output.txt";
		Logger log_file_(filename);

		log_file_.log("Exception in MAIN: {}", e.what());
	}
}
