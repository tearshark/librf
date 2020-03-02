
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

using namespace resumef;

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

static future_t<int64_t> wait_get_long(int64_t val)
{
	co_return co_await async_get_long(val);
}

//这种情况下，会生成对应的 frame-context，一个promise_type被内嵌在frame-context里
static future_t<int64_t> resumable_get_long(int64_t val)
{
	std::cout << val << std::endl;
	val = co_await wait_get_long(val);
	std::cout << val << std::endl;
	val = co_await wait_get_long(val);
	std::cout << val << std::endl;
	val = co_await wait_get_long(val);
	std::cout << val << std::endl;
	co_return val;
}

static future_t<int64_t> loop_get_long(int64_t val)
{
	std::cout << val << std::endl;
	for (int i = 0; i < 5; ++i)
	{
		val = co_await async_get_long(val);
		std::cout << val << std::endl;
	}
	co_return val;
}

void resumable_main_cb()
{
	//由于使用者可能不能明确的区分是resume function返回的awaitor还是awaitable function返回的awaitor
	//导致均有可能加入到协程里去调度。
	//所以，协程调度器应该需要能处理这种情况。
	go async_get_long(3);
	resumef::this_scheduler()->run_until_notask();

	GO
	{
		auto val = co_await resumable_get_long(2);
		std::cout << "GO:" << val << std::endl;
	};

	go loop_get_long(3);
	resumef::this_scheduler()->run_until_notask();
}
