
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>
#include <deque>
#include <mutex>

#include "librf.h"

using namespace resumef;
using namespace std::chrono;

const size_t MAX_CHANNEL_QUEUE = 5;		//0, 1, 5, 10, -1

future_t<> test_channel_read(const channel_t<std::string> & c)
{
	using namespace std::chrono;

	for (size_t i = 0; i < 10; ++i)
	{
#ifndef __clang__
		try
#endif
		{
			auto val = co_await c.read();
			//auto val = co_await c;		//第二种从channel读出数据的方法。利用重载operator co_await()，而不是c是一个awaitable_t。

			std::cout << val << ":";
#if _DEBUG
			for (auto v2 : c.debug_queue())
				std::cout << v2 << ",";
#endif
			std::cout << std::endl;
		}
#ifndef __clang__
		catch (resumef::channel_exception& e)
		{
			//MAX_CHANNEL_QUEUE=0,并且先读后写，会触发read_before_write异常
			std::cout << e.what() << std::endl;
		}
#endif

		co_await sleep_for(50ms);
	}
}

future_t<> test_channel_write(const channel_t<std::string> & c)
{
	using namespace std::chrono;

	for (size_t i = 0; i < 10; ++i)
	{
		co_await c.write(std::to_string(i));
		//co_await (c << std::to_string(i));				//第二种写入数据到channel的方法。因为优先级关系，需要将'c << i'括起来
		std::cout << "<" << i << ">:";
		
#if _DEBUG
		for (auto val : c.debug_queue())
			std::cout << val << ",";
#endif
		std::cout << std::endl;
	}
}


void test_channel_read_first()
{
	channel_t<std::string> c(MAX_CHANNEL_QUEUE);

	go test_channel_read(c);
	go test_channel_write(c);

	this_scheduler()->run_until_notask();
}

void test_channel_write_first()
{
	channel_t<std::string> c(MAX_CHANNEL_QUEUE);

	go test_channel_write(c);
	go test_channel_read(c);

	this_scheduler()->run_until_notask();
}

static const int N = 1000000;

void test_channel_performance()
{
	channel_t<int> c{1};

	go[&]() -> future_t<>
	{
		for (int i = N - 1; i >= 0; --i)
		{
			co_await(c << i);
		}
	};
	go[&]() -> future_t<>
	{
		auto tstart = high_resolution_clock::now();

		int i;
		do
		{
			i = co_await c;
		} while (i > 0);

		auto dt = duration_cast<duration<double>>(high_resolution_clock::now() - tstart).count();
		std::cout << "channel w/r " << N << " times, cost time " << dt << "s" << std::endl;
	};

	this_scheduler()->run_until_notask();
}

void resumable_main_channel()
{
	test_channel_read_first();
	std::cout << std::endl;

	test_channel_write_first();
	std::cout << std::endl;

	test_channel_performance();
}
