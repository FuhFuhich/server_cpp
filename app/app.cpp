#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <thread>
#include <fstream>
#include <sstream>
#include <atomic>
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
namespace process = boost::process;
namespace filesystem = boost::filesystem;

using tcp = asio::ip::tcp;

std::atomic<bool> server_active(false);
std::unique_ptr<process::child> server_process = nullptr;
std::string project_path = filesystem::current_path().string();
std::string output_file_path = "server_output.txt";

void start_server()
{
	if (!server_active)
	{
		server_active = true;
		server_process = std::make_unique<process::child>(
			process::search_path("server"), // Исполняемый файл
			process::std_out > output_file_path,
			process::std_err > output_file_path,
			process::start_dir = project_path
		);
	}
}

void stop_server()
{
	if (server_active && server_process && server_process->running())
	{
		server_process->terminate();
		server_active = false;
	}
}

bool has_changes()
{
	process::ipstream fetch_output;
	process::system("git fetch", process::std_out > fetch_output);
	process::ipstream local_output, remote_output;
	std::ostringstream local_head, remote_head;

	process::system("git rev-parse HEAD", process::std_out > local_output);
	process::system("git rev-parse FETCH_HEAD", process::std_out > remote_output);

	// Чтение вывода из потоков
	local_head << local_output.rdbuf();
	remote_head << remote_output.rdbuf();

	return local_head.str() != remote_head.str();
}

void pull_and_restart()
{
	if (has_changes())
	{
		stop_server();
		process::system("git reset --hard");
		process::system("git clean -fd");
		process::system("git pull --force");
		start_server();
	}
}

std::string read_logs()
{
	std::ifstream log_file(output_file_path);
	std::ostringstream buffer;
	if (log_file.is_open())
	{
		buffer << log_file.rdbuf();
	}
	return buffer.str();
}

void handle_request(http::request<http::string_body>& req, http::response<http::string_body>& res)
{
	if (req.target() == "/turn_on")
	{
		pull_and_restart();
		start_server();
		res.result(http::status::ok);
		res.body() = "Server turned on.";
	}
	else if (req.target() == "/turn_off")
	{
		stop_server();
		res.result(http::status::ok);
		res.body() = "Server turned off.";
	}
	else if (req.target() == "/get_logs")
	{
		res.result(http::status::ok);
		res.body() = read_logs();
	}
	else if (req.target() == "/get_status")
	{
		res.result(http::status::ok);
		res.body() = server_active ? "Active" : "Inactive";
	}
	else
	{
		res.result(http::status::not_found);
		res.body() = "Not Found";
	}
	res.prepare_payload();
}

void server_thread(asio::io_context& ioc, unsigned short port)
{
	tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), port));
	while (true)
	{
		tcp::socket socket(ioc);
		acceptor.accept(socket);

		beast::flat_buffer buffer;
		http::request<http::string_body> req;
		http::response<http::string_body> res;
		beast::error_code ec;

		http::read(socket, buffer, req, ec);
		if (!ec)
		{
			handle_request(req, res);
			http::write(socket, res, ec);
		}
	}
}

int main()
{
	const unsigned short port = 5400;

	std::ofstream(output_file_path).close();

	asio::io_context ioc;
	std::thread server([&]() {
		server_thread(ioc, port);
		});

	while (true)
	{
		try
		{
			pull_and_restart();
			std::cout << "Pull and restart successful." << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::minutes(5));
	}

	server.join();
}
