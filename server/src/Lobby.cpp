#include "Lobby.h"

Lobby::Lobby(boost::asio::io_context& io_context, const short& port)
    : acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
    log_file_("server_output.txt")
{
    try
    {
        log_file_.log("WebSocket server started on port: {}", port);
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
                on_accept(ec, socket);
            });
    }
    catch (const std::exception& e)
    {
        log_file_.log("Exception in Lobby start_accept: {}", e.what());
    }
}

void Lobby::on_accept(boost::system::error_code ec, std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    if (!ec)
    {
        log_file_.log("Client connected");
        do_websocket_session(socket);
    }
    else
    {
        log_file_.log("Error accepting client: {}", ec.message());
    }

    start_accept();
}

void Lobby::do_websocket_session(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    // Создаем WebSocket stream поверх TCP-сокета
    auto ws = std::make_shared<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>(std::move(*socket));

    // Выполняем WebSocket handshake
    ws->async_accept(
        [this, ws](boost::system::error_code ec)
        {
            if (!ec)
            {
                start_read(ws);
            }
            else
            {
                log_file_.log("WebSocket handshake failed: {}", ec.message());
            }
        });
}

void Lobby::start_read(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws)
{
    try
    {
        auto buffer = std::make_shared<boost::beast::flat_buffer>();

        // Читаем WebSocket сообщение в буфер
        ws->async_read(
            *buffer,
            [this, ws, buffer](boost::system::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    // Формат отправки сообщения:
                    // <Название метода внутри SqlCommander для обращения к бд> <requestId> <Данные для метода внутри SqlCommander> ... <Данные для метода внутри SqlCommander>
                    request_ = boost::beast::buffers_to_string(buffer->data());
                    log_file_.log("Received: {}", request_);

                    string_splitting(request_);
                    sql_.execute_sql_command(requests_);

                    // Логика для запроса к бд

                    requests_.clear();

                    // Вот тут реализовать возврат
                    send_message(ws, std::string(boost::beast::buffers_to_string(buffer->data())));

                    this->start_read(ws);
                }
                else
                {
                    log_file_.log("SOCKET is close");
                    ws->next_layer().close();
                    log_file_.log("Client disconnect: {}", ec.message());
                }
            });
    }
    catch (const std::exception& e)
    {
        log_file_.log("Exception in Lobby start_read: {}", e.what());
    }
}

void Lobby::send_message(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws, const std::string& message)
{
    try
    {
        auto buffer = std::make_shared<std::string>(message);

        // Записываем сообщения
        ws->async_write(
            boost::asio::buffer(*buffer),
            [this, ws, buffer](boost::system::error_code ec, std::size_t)
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
