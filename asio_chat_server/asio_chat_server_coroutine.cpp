// asioservercallback.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <cstdlib>
#include <array>
#include <memory>
#include <utility>
#include <algorithm>
#include <iostream>

#include "asio.hpp"
#include "librf.h"

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
		write_buff_[size + length + 1] = 0;

		return size + length + 1;
	}

	void start()
	{
		do_read([this](size_t size)
		{
			do_write(prepare_write_msg("first logic result : ", size), [this]
			{
				do_read([this](size_t size)
				{
					do_write(prepare_write_msg("second logic result : ", size), [this]
					{
						do_read([this](size_t size)
						{
							do_write(prepare_write_msg("third logic result : ", size), [this]
							{

							});
						});
					});
				});
			});
		});
	}

private:
	template<class _Fx>
	void do_read(_Fx && fn)
	{
		auto self(shared_from_this());
		asio::async_read_until(socket_, read_stream_, 0,
			[this, self, fn = std::forward<_Fx>(fn)](const asio::error_code& ec, std::size_t size)
		{
			if (!ec && size > 0)
			{
				auto bufs = read_stream_.data();
				std::copy(asio::buffers_begin(bufs), asio::buffers_end(bufs), read_buff_.begin());
				fn(asio::buffer_size(bufs));
			}
		});
	}

	template<class _Fx>
	void do_write(size_t size, _Fx && fn)
	{
		auto self(shared_from_this());
		asio::async_write(socket_,
			asio::buffer(write_buff_.data(), size),
			[this, self, fn = std::forward<_Fx>(fn)](std::error_code ec, std::size_t)
		{
			if (!ec)
			{
				fn();
			}
		});
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
	void do_accept()
	{
		acceptor_.async_accept(socket_,
			[this](std::error_code ec)
		{
			if (!ec)
			{
				std::make_shared<chat_session>(std::move(socket_))->start();
			}

			do_accept();
		});
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
