#include "Lobby.h"

int main()
{
	try
	{
		setlocale(0, "");

		boost::asio::io_context io_context;
		short port = 5400;
		Lobby Lobby(io_context, port);

		std::vector<std::thread> threads;
		int thread_count = std::thread::hardware_concurrency() / 2; // Может 0 потоков вернуть
		if (thread_count == 0) thread_count = 1;

		for (int i = 0; i < thread_count; ++i)
		{
			// поток передается в threads и каждый поток выполняет io_context.run()
			threads.emplace_back([&io_context]() {io_context.run(); });
		}

		for (auto& thread : threads)
		{
			// ждем, пока все потоки завершат свою работу
			thread.join();
		}
	}
	catch (const std::exception& e)
	{
		std::string filename = "server_output.txt";
		Logger log_file_(filename);

		log_file_.log("Exception in MAIN: {}", e.what());
	}
}
