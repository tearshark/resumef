// asioservercallback.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <cstdlib>
#include <chrono>
#include <thread>
#include <memory>
#include <utility>
#include <algorithm>
#include <iostream>
#include <conio.h>

#include <vector>
#include "awaituv.h"

auto async_heavy_computing_tasks(int64_t val)
{
	using namespace std::chrono;

	awaituv::promise_t<int64_t> awaitable;

	std::thread([val, st = awaitable._state->lock()]
	{
		std::this_thread::sleep_for(500ms);
		st->set_value(val * val);
		st->unlock();
	}).detach();

	return awaitable.get_future();
}

awaituv::future_t<void> heavy_computing_sequential(int64_t val)
{
	std::cout << val << " @" << std::this_thread::get_id() << std::endl;
	val = co_await async_heavy_computing_tasks(val);
	std::cout << val << " @" << std::this_thread::get_id() << std::endl;
	val = co_await async_heavy_computing_tasks(val);
	std::cout << val << " @" << std::this_thread::get_id() << std::endl;
	val = co_await async_heavy_computing_tasks(val);
	std::cout << val << " @" << std::this_thread::get_id() << std::endl;
}

awaituv::future_t<int64_t> heavy_computing_loop(int64_t val)
{
	std::cout << val << " @" << std::this_thread::get_id() << std::endl;
	for (int i = 0; i < 5; ++i)
	{
		val = co_await async_heavy_computing_tasks(val);
		std::cout << val << " @" << std::this_thread::get_id() << std::endl;
	}
	return val;
}

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
	std::cout << "main thread id is " << std::this_thread::get_id() << std::endl;

	heavy_computing_sequential(2);
	//heavy_computing_loop(2);

	_getch();
	return 0;
}
