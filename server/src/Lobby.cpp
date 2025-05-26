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

std::string Lobby::string_splitting(std::string& source)
{
    //boost::split(requests_, request, boost::is_any_of(" "), boost::token_compress_on);

    size_t pos = source.find(' ');

    if (pos == std::string::npos)
    {
        std::string cmd = source;
        source.clear();
        return cmd;
    }
    std::string cmd = source.substr(0, pos);
    source.erase(0, pos + 1);
    return cmd;
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
    auto session = std::make_shared<ClientSession>();
    session->ws = ws;

    // Выполняем WebSocket handshake
    ws->async_accept(
        [this, session](boost::system::error_code ec)
        {
            if (!ec)
            {
                start_read(session);
            }
            else
            {
                log_file_.log("WebSocket handshake failed: {}", ec.message());
            }
        });
}

void Lobby::start_read(std::shared_ptr<ClientSession> session)
{
    try
    {
        auto buffer = std::make_shared<boost::beast::flat_buffer>();

        // Читаем WebSocket сообщение в буфер
        session->ws->async_read(
            *buffer,
            [this, session, buffer](boost::system::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    std::string type = "";
                    std::string profile_id = "";

                    payload = boost::beast::buffers_to_string(buffer->data());
                    log_file_.log("Received: {}", payload);
                    std::cout << payload << "\n";

                    type = string_splitting(payload);
                    profile_id = string_splitting(payload);

                    std::string reply = sql_.execute_sql_command(type, profile_id == "-1" ? "" : profile_id, payload);
                    log_file_.log("SQL команда вернула: '{}'", reply);

                    if (!reply.empty())
                    {
                        send_message(session, reply);
                    }

                    this->start_read(session);
                }
                else
                {
                    log_file_.log("SOCKET is close");
                    session->ws->next_layer().close();
                    log_file_.log("Client disconnect: {}", ec.message());
                }
            });
    }
    catch (const std::exception& e)
    {
        log_file_.log("Exception in Lobby start_read: {}", e.what());
    }
}

void Lobby::do_write(std::shared_ptr<ClientSession> session)
{
    if (session->write_queue.empty())
    {
        session->writing = false;
        return;
    }

    session->writing = true;
    auto buffer = session->write_queue.front();

    session->ws->async_write(
        boost::asio::buffer(*buffer),
        [this, session, buffer](boost::system::error_code ec, std::size_t bytes_sent)
        {
            session->write_queue.pop();

            if (!ec)
            {
                log_file_.log("Message sent to client: {}", *buffer);
                log_file_.log("Отправлено байт: {}", bytes_sent);
            }
            else
            {
                log_file_.log("Failed to send message: {}", ec.message());
            }

            do_write(session); // Доходим до конца очереди, чтобы все отправилось
        });
}

void Lobby::send_message(std::shared_ptr<ClientSession> session, const std::string& message)
{
    log_file_.log("Попытка отправить сообщение: {}", message);
    auto buffer = std::make_shared<std::string>(message);
    session->write_queue.push(buffer);

    // Если уже отправляется сообщение, то мы просто добавили в очередь отправки
    if (!session->writing)
    {
        do_write(session);
    }
}
