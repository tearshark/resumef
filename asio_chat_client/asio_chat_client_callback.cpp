// asioservercallback.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <cstdlib>
#include <array>
#include <memory>
#include <utility>
#include <algorithm>
#include <iostream>
#include <conio.h>

#include "asio.hpp"

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
	chat_session(asio::io_service & ios, tcp::resolver::iterator ep)
		: socket_(ios)
		, endpoint_(ep)
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

	void start()
	{
		do_connect([this] 
		{
			do_write(prepare_write_msg("1 :", 0), [this]
			{
				do_read([this](size_t size)
				{
					std::cout << read_buff_.data() << std::endl;
					do_write(prepare_write_msg("2 :", 0), [this]
					{
						do_read([this](size_t size)
						{
							std::cout << read_buff_.data() << std::endl;
							do_write(prepare_write_msg("3 :", 0), [this]
							{
								do_read([this](size_t size)
								{
									std::cout << read_buff_.data() << std::endl;

									//无限不循环......
								});
							});
						});
					});
				});
			});
		});
	}

private:
	template<class _Fx>
	void do_connect(_Fx && fn)
	{
		auto self = this->shared_from_this();
		asio::async_connect(socket_, endpoint_,
			[this, self, fn = std::forward<_Fx>(fn)](std::error_code ec, tcp::resolver::iterator iter)
		{
			if (!ec)
			{
				fn();
			}
		});
	}

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
				read_stream_.consume(asio::buffer_size(bufs));

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
	tcp::resolver::iterator endpoint_;

	asio::streambuf read_stream_;
	std::array<char, 4096 * 16> read_buff_;

	std::array<char, 4096 * 16> write_buff_;
};

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
	try
	{
		asio::io_service ios;

		asio::ip::tcp::resolver resolver_(ios);
		asio::ip::tcp::resolver::query query_("127.0.0.1", "3456");
		tcp::resolver::iterator iter = resolver_.resolve(query_);

		auto chat = std::make_shared<chat_session>(ios, iter);
		chat->start();

		ios.run();

		_getch();
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << "\n";
	}

	return 0;
}
