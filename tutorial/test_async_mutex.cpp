
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>
#include <deque>

#include "librf.h"

using namespace resumef;
using namespace std::chrono;

static mutex_t g_lock;
static intptr_t g_counter = 0;

static const size_t N = 10;

//🔒-50ms-🔒🗝🗝-150ms-|
//-------------.........
static future_t<> test_mutex_pop(size_t idx)
{
	for (size_t i = 0; i < N / 2; ++i)
	{
		{
			auto _locker = co_await g_lock.lock();	//_locker析构后，会调用对应的unlock()函数。

			--g_counter;
			std::cout << "pop :" << g_counter << " on " << idx << std::endl;

			co_await 50ms;

			auto _locker_2 = co_await g_lock.lock();

			--g_counter;
			std::cout << "pop :" << g_counter << " on " << idx << std::endl;
		}
		co_await 150ms;
	}
}

//🔒-50ms-🗝-50ms-🔒-50ms-🗝-50ms-|
//---------........---------.......
static future_t<> test_mutex_push(size_t idx)
{
	for (size_t i = 0; i < N; ++i)
	{
		{
			auto _locker = co_await g_lock;

			++g_counter;
			std::cout << "push:" << g_counter << " on " << idx << std::endl;

			co_await 50ms;
		}
		co_await 50ms;
	}
}

//🔒-50ms-🗝-50ms-🔒-50ms-🗝-50ms-|
//---------........---------.......
static std::thread test_mutex_async_push(size_t idx)
{
	return std::thread([=]
	{
		char provide_unique_address = 0;
		for (size_t i = 0; i < N; ++i)
		{
			{
				auto _locker = g_lock.lock(&provide_unique_address);

				++g_counter;
				std::cout << "push:" << g_counter << " on " << idx << std::endl;
				std::this_thread::sleep_for(50ms);
			}

			std::this_thread::sleep_for(50ms);
		}
	});
}

static void resumable_mutex_synch()
{
	go test_mutex_push(0);
	go test_mutex_pop(1);

	this_scheduler()->run_until_notask();

	std::cout << "result:" << g_counter << std::endl;
}

static void resumable_mutex_async()
{
	auto th = test_mutex_async_push(0);
	std::this_thread::sleep_for(25ms);
	go test_mutex_pop(1);

	this_scheduler()->run_until_notask();
	th.join();

	std::cout << "result:" << g_counter << std::endl;
}

void resumable_main_mutex()
{
	resumable_mutex_synch();
	std::cout << std::endl;

	resumable_mutex_async();
}
