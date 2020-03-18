
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>
#include <deque>

#include "librf.h"

using namespace resumef;

mutex_t g_lock;
std::deque<size_t> g_queue;

future_t<> test_mutex_pop(size_t idx)
{
	using namespace std::chrono;

	for (size_t i = 0; i < 10; ++i)
	{
		auto _locker = co_await g_lock.lock();

		if (g_queue.size() > 0)
		{
			size_t val = g_queue.front();
			g_queue.pop_front();

			std::cout << val << " on " << idx << std::endl;
		}
		co_await sleep_for(500ms);
	}
}

future_t<> test_mutex_push(size_t idx)
{
	using namespace std::chrono;

	for (size_t i = 0; i < 10; ++i)
	{
		auto _locker = co_await g_lock.lock();
		g_queue.push_back(i);
		std::cout << i << " on " << idx << std::endl;

		co_await sleep_for(500ms);
	}
}

void resumable_main_mutex()
{
	go test_mutex_push(0);
	go test_mutex_pop(1);

	this_scheduler()->run_until_notask();
}
