#pragma once

#include "Logger.h"
#include "BufferPool.h"
#include "SqlCommander.h"
#include "utf8.h"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <chrono>
#include <vector>
#include <queue>

using boost::asio::ip::tcp;

class Lobby 
{
public:
	Logger log_file_;

private:
	std::queue<std::shared_ptr<std::string>> write_queue_;
	bool writing_ = false;
	boost::asio::ip::tcp::acceptor acceptor_;
	BufferPool buffer_pool_;
	SqlCommander sql_;
	std::vector<std::string> requests_;
	std::string payload;

public:
	Lobby() = delete; // Запрещаем создавать конструктор по умолчанию явно
	Lobby(boost::asio::io_context& io_context, const short& port);
	~Lobby();

private:
	void start_accept();
	void on_accept(boost::system::error_code ec, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
	void do_websocket_session(std::shared_ptr<boost::asio::ip::tcp::socket> socket);
	void start_read(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws);
	void send_message(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws, const std::string& message);
	std::string string_splitting(std::string& request);
	void do_write(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws);
};
