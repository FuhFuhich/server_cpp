#include <boost/asio.hpp>

#include <iostream>

using boost::asio::ip::tcp;

class Server
{
private:
	tcp::acceptor acceptor_;

public:
	Server(boost::asio::io_context& io_context, short port)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
	{
		try
		{
			start_accept();
		}
		catch (const std::exception& e)
		{
			std::cerr << "Exception in SERVER constructor: " << e.what() << "\n";
		}
	}

private:
	void start_accept()
	{
		try
		{
			std::cout << "start_accept()...\n";
			auto socket = std::make_shared<tcp::socket>(acceptor_.get_executor());

			acceptor_.async_accept(*socket,
				[this, socket](boost::system::error_code ec)
				{
					if (!ec)
					{
						std::cout << "Client is connected\n";
						start_read(socket);
					}
					else
					{
						std::cerr << "Error accepting client: " << ec.message() << "\n";
					}

					start_accept();
				});
		}
		catch (const std::exception& e)
		{
			std::cerr << "Exception in SERVER start_accept: " << e.what() << "\n";
		}
	}

	void start_read(std::shared_ptr<tcp::socket> socket)
	{
		try
		{
			auto buffer = std::make_shared<std::array<char, 1024>>(); // буффер в 1024*8 бита, т.е. 1024 символа

			socket->async_read_some(boost::asio::buffer(*buffer),
				[this, socket, buffer](boost::system::error_code ec, std::size_t length) // Указываем на текущий объект для того, чтобы был доступ к классу и его объектам Server
				{
					if (!ec)
					{
						std::cout << "Received: " << std::string(buffer->data(), length) << "\n";
						send_message(socket, std::string(buffer->data(), length));
						this->start_read(socket); // Зацикливаем прослушивание, т.е. ждем следующее сообщение

					}
					else
					{
						std::cout << "Client disconnect\n";
						socket->close(); // Закрываем сокет, если клиент отключился
					}
				});
		}
		catch (const std::exception& e)
		{
			std::cerr << "Exception in SERVER start_read: " << e.what() << "\n";
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
				[socket, buffer](boost::system::error_code ec, std::size_t)
				{
					if (!ec)
					{
						std::cout << "Message sent to client: " << *buffer << "\n";
					}
					else
					{
						std::cerr << "Failed to send message: " << ec.message() << "\n";
					}
				});
		}
		catch (const std::exception& e)
		{
			std::cerr << "Exception in SERVER send_message: " << e.what() << "\n";
		}
	}
};

int main()
{
	try
	{
		setlocale(0, "");
		boost::asio::io_context io_context;
		Server server(io_context, 8080);
		io_context.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception in MAIN: " << e.what() << "\n";
	}
}
