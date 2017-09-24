
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>
#include <deque>
#include <mutex>

#include "librf.h"

using namespace resumef;

mutex_t g_lock;
std::deque<size_t> g_queue;

future_vt test_mutex_pop(size_t idx)
{
	using namespace std::chrono;

	for (size_t i = 0; i < 10; ++i)
	{
		co_await resumf_guard_lock(g_lock);

		if (g_queue.size() > 0)
		{
			size_t val = g_queue.front();
			g_queue.pop_front();

			std::cout << val << " on " << idx << std::endl;
		}
		co_await sleep_for(500ms);
	}
}

future_vt test_mutex_push()
{
	using namespace std::chrono;

	for (size_t i = 0; i < 10; ++i)
	{
		co_await resumf_guard_lock(g_lock);
		g_queue.push_back(i);

		co_await sleep_for(500ms);
	}
}

void resumable_main_mutex()
{
	go test_mutex_push();
	go test_mutex_pop(1);

	g_scheduler.run_until_notask();
}
