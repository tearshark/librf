
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>
#include <deque>
#include <mutex>

#include "librf.h"

using namespace resumef;

const size_t MAX_CHANNEL_QUEUE = 0;		//0, 1, 5, 10, -1

future_vt test_channel_read(const channel_t<size_t> & c)
{
	using namespace std::chrono;

	for (size_t i = 0; i < 10; ++i)
	{
		try
		{
			//auto val = co_await c.read();
			auto val = co_await c;		//第二种从channel读出数据的方法。利用重载operator co_await()，而不是c是一个awaitable_t。

			std::cout << val << ":";
#if _DEBUG
			for (auto val : c.debug_queue())
				std::cout << val << ",";
#endif
			std::cout << std::endl;
		}
		catch (channel_exception e)
		{
			//MAX_CHANNEL_QUEUE=0,并且先读后写，会触发read_before_write异常
			std::cout << e.what() << std::endl;
		}

		co_await sleep_for(50ms);
	}
}

future_vt test_channel_write(const channel_t<size_t> & c)
{
	using namespace std::chrono;

	for (size_t i = 0; i < 10; ++i)
	{
		//co_await c.write(i);
		co_await (c << i);				//第二种写入数据到channel的方法。因为优先级关系，需要将'c << i'括起来
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
	channel_t<size_t> c(MAX_CHANNEL_QUEUE);

	go test_channel_read(c);
	go test_channel_write(c);

	g_scheduler.run_until_notask();
}

void test_channel_write_first()
{
	channel_t<size_t> c(MAX_CHANNEL_QUEUE);

	go test_channel_write(c);
	go test_channel_read(c);

	g_scheduler.run_until_notask();
}

void resumable_main_channel()
{
	test_channel_read_first();
	std::cout << std::endl;

	test_channel_write_first();
}
