#pragma once

#include "Logger.h"
#include "BufferPool.h"
#include "SqlCommander.h"

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio/ssl.hpp>

#include <iostream>
#include <chrono>
#include <vector>

using boost::asio::ip::tcp;

class Lobby 
{
public:
	Logger log_file_;

private:
	tcp::acceptor acceptor_;
	std::vector<std::string> requests_;
	std::string request_;
	boost::asio::ssl::context ssl_context_;
	BufferPool buffer_pool_;
	SqlCommander sql_;

public:
	Lobby() = delete;
	Lobby(boost::asio::io_context& io_context, const short& port);
	~Lobby();

private:
	void start_accept();
	void start_read(std::shared_ptr<boost::asio::ssl::stream<tcp::socket>> ssl_socket);
	void send_message(std::shared_ptr<boost::asio::ssl::stream<tcp::socket>> ssl_socket, const std::string& message);
	void string_splitting(const std::string& request);
};
