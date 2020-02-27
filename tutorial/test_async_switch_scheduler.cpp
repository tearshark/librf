
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

using namespace resumef;

static scheduler_t* sch_in_main = nullptr;
static scheduler_t* sch_in_thread = nullptr;

void run_in_thread(channel_t<bool>& c_done)
{
	std::cout << "other thread = " << std::this_thread::get_id() << std::endl;

	local_scheduler my_scheduler;
	sch_in_thread = this_scheduler();

	c_done << true;

	sch_in_thread->run();
}

template<class _Ctype>
static void callback_get_long(int64_t val, _Ctype&& cb)
{
	using namespace std::chrono;
	std::thread([val, cb = std::forward<_Ctype>(cb)]
		{
			std::this_thread::sleep_for(500ms);
			cb(val * val);
		}).detach();
}

//这种情况下，没有生成 frame-context，因此，并没有promise_type被内嵌在frame-context里
static future_t<int64_t> async_get_long(int64_t val)
{
	resumef::awaitable_t<int64_t> awaitable;
	callback_get_long(val, [awaitable](int64_t val)
	{
		awaitable.set_value(val);
	});
	return awaitable.get_future();
}

//这种情况下，会生成对应的 frame-context，一个promise_type被内嵌在frame-context里
static future_t<> resumable_get_long(int64_t val, channel_t<bool> & c_done)
{
	std::cout << "thread = " << std::this_thread::get_id() << ", value = " << val << std::endl;

	co_await via(sch_in_thread);
	val = co_await async_get_long(val);
	std::cout << "thread = " << std::this_thread::get_id() << ", value = " << val << std::endl;

	co_await *sch_in_main;
	val = co_await async_get_long(val);
	std::cout << "thread = " << std::this_thread::get_id() << ", value = " << val << std::endl;

	co_await *sch_in_thread;
	val = co_await async_get_long(val);
	std::cout << "thread = " << std::this_thread::get_id() << ", value = " << val << std::endl;

	c_done << true;
}

void resumable_main_switch_scheduler()
{
	sch_in_main = this_scheduler();
	channel_t<bool> c_done{ 1 };

	std::cout << "main thread = " << std::this_thread::get_id() << std::endl;
	std::thread(&run_in_thread, std::ref(c_done)).detach();

	GO
	{
		co_await c_done;
		go resumable_get_long(3, c_done);
		co_await c_done;
	};

	sch_in_main->run_until_notask();
}
