
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"


auto async_get_long(int64_t val)
{
	using namespace std::chrono;

	resumef::promise_t<int64_t> awaitable;

	std::thread([val, st = awaitable._state->lock()]
	{
		std::this_thread::sleep_for(500ms);
		st->set_value(val * val);
		st->unlock();
	}).detach();

	return awaitable.get_future();
}

resumef::future_vt resumable_get_long(int64_t val)
{
	std::cout << val << std::endl;
	val = co_await async_get_long(val);
	std::cout << val << std::endl;
	val = co_await async_get_long(val);
	std::cout << val << std::endl;
	val = co_await async_get_long(val);
	std::cout << val << std::endl;
}

resumef::future_t<int64_t> loop_get_long(int64_t val)
{
	std::cout << val << std::endl;
	for (int i = 0; i < 5; ++i)
	{
		val = co_await async_get_long(val);
		std::cout << val << std::endl;
	}
	return val;
}

void resumable_main_cb()
{
	std::cout << std::this_thread::get_id() << std::endl;

	go []()->resumef::future_vt
	{
		auto val = co_await loop_get_long(2);
		std::cout << val << std::endl;
	};
	//resumef::g_scheduler.run_until_notask();

	go resumable_get_long(3);
	resumef::g_scheduler.run_until_notask();
}
