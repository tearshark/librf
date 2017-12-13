//验证channel是否线程安全

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
static std::mutex cout_mutex;

#define OUTPUT_DEBUG	0

future_vt test_channel_consumer(const channel_t<std::string> & c, size_t cnt)
{
	for (size_t i = 0; i < cnt; ++i)
	{
		try
		{
			auto val = co_await c.read();
#if OUTPUT_DEBUG
			{
				scoped_lock<std::mutex> __lock(cout_mutex);
				std::cout << "R " << val << "@" << std::this_thread::get_id() << std::endl;
			}
#endif
		}
		catch (channel_exception e)
		{
			//MAX_CHANNEL_QUEUE=0,并且先读后写，会触发read_before_write异常
			scoped_lock<std::mutex> __lock(cout_mutex);
			std::cout << e.what() << std::endl;
		}

#if OUTPUT_DEBUG
		co_await sleep_for(50ms);
#endif
	}
}

future_vt test_channel_producer(const channel_t<std::string> & c, size_t cnt)
{
	for (size_t i = 0; i < cnt; ++i)
	{
		co_await c.write(std::to_string(i));
#if OUTPUT_DEBUG
		{
			scoped_lock<std::mutex> __lock(cout_mutex);
			std::cout << "W " << i << "@" << std::this_thread::get_id() << std::endl;
		}
#endif
	}
}

const size_t N = 8;
const size_t BATCH = 1000000;
const size_t MAX_CHANNEL_QUEUE = N + 1;		//0, 1, 5, 10, -1

void resumable_main_channel_mult_thread()
{
	channel_t<std::string> c(MAX_CHANNEL_QUEUE);

	std::thread wth([&]
	{
		//local_scheduler my_scheduler;		//2017/11/27日，仍然存在BUG。真多线程下调度，存在有协程无法被调度完成的BUG
		go test_channel_producer(c, BATCH * N);
		this_scheduler()->run_until_notask();

		std::cout << "Write OK" << std::endl;
	});

	//std::this_thread::sleep_for(100ms);

	std::thread rth[N];
	for (size_t i = 0; i < N; ++i)
	{
		rth[i] = std::thread([&]
		{
			//local_scheduler my_scheduler;		//2017/11/27日，仍然存在BUG。真多线程下调度，存在有协程无法被调度完成的BUG
			go test_channel_consumer(c, BATCH);
			this_scheduler()->run_until_notask();

			std::cout << "Read OK" << std::endl;
		});
	}

	for(auto & th : rth)
		th.join();
	wth.join();

	std::cout << "OK" << std::endl;
	_getch();
}
