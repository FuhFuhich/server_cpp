#include "Lobby.h";

Lobby::Lobby(boost::asio::io_context& io_context, const short& port)
	: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
	ssl_context_(boost::asio::ssl::context::sslv23),
	log_file_("server_output.txt")
{
	try
	{
		ssl_context_.set_options(boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::single_dh_use);

		ssl_context_.use_certificate_chain_file("certs/server.crt");
		ssl_context_.use_private_key_file("certs/server.key", boost::asio::ssl::context::pem);

		log_file_.log("SSL server started on port: " + std::to_string(port));
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

		auto ssl_socket = std::make_shared<boost::asio::ssl::stream<tcp::socket>>(acceptor_.get_executor(), ssl_context_);

		acceptor_.async_accept(ssl_socket->lowest_layer(),
			[this, ssl_socket](boost::system::error_code ec)
			{
				if (!ec)
				{
					log_file_.log("Client is connected");

					ssl_socket->async_handshake(boost::asio::ssl::stream_base::server,
						[this, ssl_socket](boost::system::error_code ec)
						{
							handle_handshake(ssl_socket, ec);
						});
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

void Lobby::handle_handshake(std::shared_ptr<boost::asio::ssl::stream<tcp::socket>> ssl_socket, const boost::system::error_code& ec)
{
	if (!ec)
	{
		log_file_.log("SSL handshake successful");
		start_read(ssl_socket);
	}
	else
	{
		log_file_.log("SSL handshake failed : " + ec.message());
	}
}

void Lobby::start_read(std::shared_ptr<boost::asio::ssl::stream<tcp::socket>> ssl_socket)
{
	try
	{
		auto buffer = std::make_shared<std::array<char, 1024>>(); // 1024*8 бита

		ssl_socket->async_read_some(boost::asio::buffer(*buffer),
			[this, ssl_socket, buffer](boost::system::error_code ec, std::size_t length)
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

					send_message(ssl_socket, std::string(buffer->data(), length));
					this->start_read(ssl_socket);
				}
				else
				{
					log_file_.log("Client disconnect: " + ec.message());
					ssl_socket->lowest_layer().close();
				}
			});
	}
	catch (const std::exception& e)
	{
		log_file_.log("Exception in Lobby start_read: " + std::string(e.what()));
	}
}

void Lobby::send_message(std::shared_ptr<boost::asio::ssl::stream<tcp::socket>> ssl_socket, const std::string& message)
{
	try
	{
		auto buffer = std::make_shared<std::string>(message);

		boost::asio::async_write(
			*ssl_socket,
			boost::asio::buffer(*buffer),
			[this, ssl_socket, buffer](boost::system::error_code ec, std::size_t)
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
