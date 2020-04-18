#include <chrono>
#include <iostream>
#include <string>
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
			batch_unlock_t _locker = co_await g_lock.lock();	//_locker析构后，会调用对应的unlock()函数。

			--g_counter;
			std::cout << "pop :" << g_counter << " on " << idx << std::endl;

			co_await 50ms;

			batch_unlock_t _locker_2 = co_await g_lock;

			--g_counter;
			std::cout << "pop :" << g_counter << " on " << idx << std::endl;
		}
		co_await 150ms;
	}
}

//🔒-50ms-🗝-50ms-🔒-50ms-🗝-50ms-|
//---------........---------.......
//方法之一
static future_t<> test_mutex_push(size_t idx)
{
	for (size_t i = 0; i < N; ++i)
	{
		{
			batch_unlock_t _locker = co_await g_lock.lock();

			++g_counter;
			std::cout << "push:" << g_counter << " on " << idx << std::endl;

			co_await 50ms;
		}
		co_await 50ms;
	}
}

static future_t<> test_mutex_try_push(size_t idx)
{
	for (size_t i = 0; i < N; ++i)
	{
		{
			for (;;)
			{
				auto result = co_await g_lock.try_lock();
				if (result) break;
				co_await yield();
			}

			++g_counter;
			std::cout << "push:" << g_counter << " on " << idx << std::endl;

			co_await 50ms;
			co_await g_lock.unlock();
		}
		co_await 50ms;
	}
}

static future_t<> test_mutex_timeout_push(size_t idx)
{
	for (size_t i = 0; i < N; ++i)
	{
		{
			for (;;)
			{
				auto result = co_await g_lock.try_lock_for(10ms);
				if (result) break;
				co_await yield();
			}

			++g_counter;
			std::cout << "push:" << g_counter << " on " << idx << std::endl;

			co_await 50ms;
			co_await g_lock.unlock();
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
			if (g_lock.try_lock_for(500ms, &provide_unique_address))
			{
				batch_unlock_t _locker(std::adopt_lock, &provide_unique_address, g_lock);

				++g_counter;
				std::cout << "push:" << g_counter << " on " << idx << std::endl;
				std::this_thread::sleep_for(50ms);

				//g_lock.unlock(&provide_unique_address);
			}

			std::this_thread::sleep_for(50ms);
		}
	});
}

static void resumable_mutex_synch()
{
	go test_mutex_timeout_push(0);
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

static future_t<> resumable_mutex_range_push(size_t idx, mutex_t a, mutex_t b, mutex_t c)
{
	for (int i = 0; i < 10000; ++i)
	{
		batch_unlock_t __lockers = co_await mutex_t::lock(a, b, c);
		assert(a.is_locked());
		assert(b.is_locked());
		assert(c.is_locked());

		++g_counter;
		//std::cout << "push:" << g_counter << " on " << idx << std::endl;
		
		//co_await 5ms;
	}
}

static future_t<> resumable_mutex_range_pop(size_t idx, mutex_t a, mutex_t b, mutex_t c)
{
	for (int i = 0; i < 10000; ++i)
	{
		co_await mutex_t::lock(adopt_manual_unlock, a, b, c);
		assert(a.is_locked());
		assert(b.is_locked());
		assert(c.is_locked());

		--g_counter;
		//std::cout << "pop :" << g_counter << " on " << idx << std::endl;

		//co_await 5ms;
		co_await mutex_t::unlock(a, b, c);
	}
}

static void resumable_mutex_lock_range()
{
	mutex_t mtxA, mtxB, mtxC;

	//不同的线程里加锁也需要是线程安全的
	std::thread push_th([&]
	{
		local_scheduler_t __ls__;

		go resumable_mutex_range_push(10, mtxA, mtxB, mtxC);
		go resumable_mutex_range_push(11, mtxA, mtxC, mtxB);

		this_scheduler()->run_until_notask();
	});

	go resumable_mutex_range_pop(12, mtxC, mtxB, mtxA);
	go resumable_mutex_range_pop(13, mtxB, mtxA, mtxC);

	this_scheduler()->run_until_notask();
	push_th.join();

	std::cout << "result:" << g_counter << std::endl;
}

void resumable_main_mutex()
{
	resumable_mutex_synch();
	std::cout << std::endl;

	resumable_mutex_async();
	std::cout << std::endl;

	resumable_mutex_lock_range();
}
