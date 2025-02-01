#pragma once

#include <boost/asio.hpp>

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <chrono>

using boost::asio::ip::tcp;

class Lobby 
{
private:
	tcp::acceptor acceptor_;
	std::ofstream log_file_;

public:
	Lobby() = delete;
	Lobby(boost::asio::io_context& io_context, const short& port);
	~Lobby();

private:
	void start_accept();
	void start_read(std::shared_ptr<tcp::socket> socket);
	void send_message(std::shared_ptr<tcp::socket> socket, const std::string& message);

};