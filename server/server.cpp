#include <boost/asio.hpp>

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <chrono>

using boost::asio::ip::tcp;

class Server
{
private:
	tcp::acceptor acceptor_;
	std::ofstream log_file_;

public:
	Server() = delete;
	Server(boost::asio::io_context& io_context, const short& port)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
	{
		try
		{
			log_file_.open("server_output.txt", std::ios::app);
			log_file_.setf(std::ios::unitbuf); // Убираем буфферизацию при записи в файл

			if (!log_file_.is_open())
			{
				throw std::runtime_error("Failed to open log file.");
			}

			start_accept();
		}
		catch (const std::exception& e)
		{
			log_file_ << "Exception in SERVER constructor: " << e.what() << "\n";
		}
	}

	~Server()
	{
		if (log_file_.is_open())
		{
			log_file_.close();
		}
	}

	std::ofstream& get_log_file()
	{
		return log_file_;
	}

private:
	void start_accept()
	{
		try
		{
			log_file_ << "Client is connected\n";

			auto socket = std::make_shared<tcp::socket>(acceptor_.get_executor());

			acceptor_.async_accept(*socket,
				[this, socket](boost::system::error_code ec)
				{
					if (!ec)
					{
						log_file_ << "Client is connected\n";
						start_read(socket);
					}
					else
					{
						log_file_ << "Error accepting client: " << ec.message() << "\n";
					}

					start_accept();
				});
		}
		catch (const std::exception& e)
		{
			log_file_ << "Exception in SERVER start_accept: " << e.what() << "\n";
		}
	}

	void start_read(std::shared_ptr<tcp::socket> socket)
	{
		try
		{
			auto buffer = std::make_shared<std::array<char, 1024>>(); // 1024*8 бита

			socket->async_read_some(boost::asio::buffer(*buffer),
				[this, socket, buffer](boost::system::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						log_file_ << "Received: " << std::string(buffer->data(), length) << "\n";

						send_message(socket, std::string(buffer->data(), length));
						this->start_read(socket); 

					}
					else
					{
						log_file_ << "Client disconnect\n";

						socket->close();
					}
				});
		}
		catch (const std::exception& e)
		{
			log_file_ << "Exception in SERVER start_read: " << e.what() << "\n";
		}
	}

	void send_message(std::shared_ptr<tcp::socket> socket, const std::string& message)
	{
		try
		{
			auto buffer = std::make_shared<std::string>(message);

			boost::asio::async_write(
				*socket,
				boost::asio::buffer(*buffer),
				[this, socket, buffer](boost::system::error_code ec, std::size_t)
				{
					if (!ec)
					{
						log_file_ << "Message sent to client: " << *buffer << "\n";
					}
					else
					{
						log_file_ << "Failed to send message: " << ec.message() << "\n";
					}
				});
		}
		catch (const std::exception& e)
		{
			log_file_ << "Exception in SERVER send_message: " << e.what() << "\n";
		}
	}
};

int main()
{
	try
	{
		setlocale(0, "");
		boost::asio::io_context io_context;
		short port = 8080;
		Server server(io_context, port);
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

		log_file_.close();
	}
}
