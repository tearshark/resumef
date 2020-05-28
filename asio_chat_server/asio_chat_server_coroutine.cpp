﻿// asioservercallback.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <cstdlib>
#include <array>
#include <memory>
#include <utility>
#include <algorithm>
#include <iostream>

#include "asio.hpp"
#include "awaituv.h"
#include "asio_use_task.hpp"
#include "librf/librf.h"

using asio::ip::tcp;

template<class iterator> 
std::pair<iterator, bool> chat_match_zero(iterator begin, iterator end)
{
  iterator i = begin;
  while (i != end)
    if (0 == *i++)
      return std::make_pair(i, true);
  return std::make_pair(i, false);
}

class chat_session : public std::enable_shared_from_this<chat_session>
{
public:
	chat_session(tcp::socket && socket)
		: socket_(std::move(socket))
	{
	}

	size_t prepare_write_msg(const char * s, size_t size)
	{
		size_t length = strlen(s);

		std::copy(s, s + length, write_buff_.begin());
		std::copy(read_buff_.begin(), read_buff_.begin() + size, write_buff_.begin() + length);
		write_buff_[size + length] = 0;

		return size + length + 1;
	}

	awaituv::future_t<void> start()
	{
		auto self = this->shared_from_this();
		try
		{
			auto size = co_await do_read();
			std::cout << read_buff_.data() << std::endl;
			co_await do_write(prepare_write_msg("first logic result : ", size));

			size = co_await do_read();
			std::cout << read_buff_.data() << std::endl;
			co_await do_write(prepare_write_msg("second logic result : ", size));

			size = co_await do_read();
			std::cout << read_buff_.data() << std::endl;
			co_await do_write(prepare_write_msg("third logic result : ", size));

			//无限不循环......
		}
		catch (...)
		{

		}
	}

private:
	awaituv::future_t<size_t> do_read()
	{
		auto size = co_await asio::async_read_until(socket_, read_stream_, 0, asio::use_task);
		auto bufs = read_stream_.data();
		std::copy(asio::buffers_begin(bufs), asio::buffers_end(bufs), read_buff_.begin());
		read_stream_.consume(asio::buffer_size(bufs));

		return asio::buffer_size(bufs);
	}

	awaituv::future_t<size_t> do_write(size_t size)
	{
		return asio::async_write(socket_,
			asio::buffer(write_buff_.data(), size), asio::use_task);
	}

	tcp::socket socket_;

	asio::streambuf read_stream_;
	std::array<char, 4096 * 16> read_buff_;

	std::array<char, 4096 * 16> write_buff_;
};

//----------------------------------------------------------------------

class chat_server
{
public:
	chat_server(asio::io_service& ios, const tcp::endpoint& endpoint)
		: acceptor_(ios, endpoint)
		, socket_(ios)
	{
		do_accept();
	}

private:
	awaituv::future_t<void> do_accept()
	{
		try
		{
			for (;;)
			{
				co_await acceptor_.async_accept(socket_, asio::use_task);
				std::make_shared<chat_session>(std::move(socket_))->start();
			}
		}
		catch (...)
		{

		}
	}

	tcp::acceptor acceptor_;
	tcp::socket socket_;
};

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
	try
	{
		asio::io_service ios;

		tcp::endpoint endpoint(tcp::v4(), 3456);
		chat_server chat = { ios, endpoint };

		ios.run();
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << "\n";
	}

	return 0;
}
