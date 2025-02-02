#include "Lobby.h";

Lobby::Lobby(boost::asio::io_context& io_context, const short& port)
	: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
	log_file_("server_output.txt")
{
	try
	{
		log_file_.log("Server started on port: " + std::to_string(port));
		start_accept();
	}
	catch (const std::exception& e)
	{
		log_file_.log("Exception in Lobby constructor: " + std::string(e.what()));
	}
}

Lobby::~Lobby()
{
	if (acceptor_.is_open())
	{
		acceptor_.close();
	}
}

void Lobby::string_splitting(const std::string& request)
{
	boost::split(requests, request, boost::is_any_of(" "), boost::token_compress_on);
}

void Lobby::start_accept()
{
	try
	{
		log_file_.log("Client is connected");

		auto socket = std::make_shared<tcp::socket>(acceptor_.get_executor());

		acceptor_.async_accept(*socket,
			[this, socket](boost::system::error_code ec)
			{
				if (!ec)
				{
					log_file_.log("Client is connected");
					start_read(socket);
				}
				else
				{
					log_file_.log("Error accepting client: " + std::string(ec.message()));
				}

				start_accept();
			});
	}
	catch (const std::exception& e)
	{
		log_file_.log("Exception in Lobby start_accept: " + std::string(e.what()));
	}
}

void Lobby::start_read(std::shared_ptr<tcp::socket> socket)
{
	try
	{
		auto buffer = std::make_shared<std::array<char, 1024>>(); // 1024*8 бита

		socket->async_read_some(boost::asio::buffer(*buffer),
			[this, socket, buffer](boost::system::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					// Формат отправки сообщения:
					// <Название метода внутри SqlCommander для обращения к бд> <requestId> <Данные для метода внутри SqlCommander> ... <Данные для метода внутри SqlCommander>
					request = std::string(buffer->data(), length);
					log_file_.log("Received: " + std::string(request));

					string_splitting(request);
					// Логика для запроса к бд
					requests.clear();

					send_message(socket, std::string(buffer->data(), length));
					this->start_read(socket);
				}
				else
				{
					log_file_.log("Client disconnect");

					if (socket->is_open()) 
					{
						socket->close();
					}
				}
			});
	}
	catch (const std::exception& e)
	{
		log_file_.log("Exception in Lobby start_read: " + std::string(e.what()));
	}
}

void Lobby::send_message(std::shared_ptr<tcp::socket> socket, const std::string& message)
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
					log_file_.log("Message sent to client: " + std::string(*buffer));
				}
				else
				{
					log_file_.log("Failed to send message: " + std::string(ec.message()));
				}
			});
	}
	catch (const std::exception& e)
	{
		log_file_.log("Exception in Lobby send_message: " + std::string(e.what()));
	}
}
