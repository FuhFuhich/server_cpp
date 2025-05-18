#include "Lobby.h";

Lobby::Lobby(boost::asio::io_context& io_context, const short& port)
    : acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      log_file_("server_output.txt")
{
    try
    {
        log_file_.log("TCP server started on port: {}", port);
        start_accept();
    }
    catch (const std::exception& e)
    {
        log_file_.log("Exception in Lobby constructor: {}", e.what());
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
	boost::split(requests_, request, boost::is_any_of(" "), boost::token_compress_on);
}

void Lobby::start_accept()
{
	try
	{
		auto socket = std::make_shared<boost::asio::ip::tcp::socket>(acceptor_.get_executor());

		// Ожидаем подключение
		acceptor_.async_accept(*socket,
			[this, socket](boost::system::error_code ec)
			{
				if (!ec)
				{
					log_file_.log("Client connected");
					start_read(socket);
				}
				else
				{
					log_file_.log("Error accepting client: {}", ec.message());
				}

				start_accept();
			});
	}
	catch (const std::exception& e)
	{
		log_file_.log("Exception in Lobby start_accept: {}", e.what());
	}
}

void Lobby::start_read(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
	try
	{
		auto buffer = buffer_pool_.get_buffer();

		socket->async_read_some(boost::asio::buffer(*buffer),
			[this, socket, buffer](boost::system::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					// Формат отправки сообщения:
					// <Название метода внутри SqlCommander для обращения к бд> <requestId> <Данные для метода внутри SqlCommander> ... <Данные для метода внутри SqlCommander>
					request_ = std::string(buffer->data(), length);
					log_file_.log("Received: {}", request_);
					std::cout << request_ << "\n\n";

					string_splitting(request_);
					sql_.execute_sql_command(requests_);

					// Логика для запроса к бд

					requests_.clear();

					send_message(socket, std::string(buffer->data(), length));

					buffer_pool_.release(buffer);

					this->start_read(socket);
				}
				else
				{
					log_file_.log("SOCKET is close");
					socket->close();
					log_file_.log("Client disconnect: {}", ec.message());
				}
			});
	}
	catch (const std::exception& e)
	{
		log_file_.log("Exception in Lobby start_read: {}", e.what());
	}
}

void Lobby::send_message(std::shared_ptr<boost::asio::ip::tcp::socket> socket, const std::string& message)
{
	try
	{
		auto buffer = std::make_shared<std::string>(message);

		// Записываем сообщения
		boost::asio::async_write(
			*socket,
			boost::asio::buffer(*buffer),
			[this, socket, buffer](boost::system::error_code ec, std::size_t)
			{
				if (!ec)
				{
					log_file_.log("Message sent to client: {}", *buffer);
				}
				else
				{
					log_file_.log("Failed to send message: {}", ec.message());
				}
			});
	}
	catch (const std::exception& e)
	{
		log_file_.log("Exception in Lobby send_message: {}", e.what());
	}
}
