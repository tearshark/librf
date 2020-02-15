
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

template<class _Ctype>
void callback_get_long(int64_t val, _Ctype&& cb)
{
	using namespace std::chrono;
	std::thread([val, cb = std::forward<_Ctype>(cb)]
		{
			std::this_thread::sleep_for(500ms);
			cb(val * val);
		}).detach();
}

//这种情况下，没有生成 frame-context，因此，并没有promise_type被内嵌在frame-context里
auto async_get_long(int64_t val)
{
	resumef::awaitable_t<int64_t> st;
	callback_get_long(val, [st](int64_t val)
	{
		st.set_value(val);
	});
	return st.get_future();
}

//这种情况下，会生成对应的 frame-context，一个promise_type被内嵌在frame-context里
resumef::future_t<> resumable_get_long(int64_t val)
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

	go []()->resumef::future_t<>
	{
		auto val = co_await loop_get_long(2);
		std::cout << val << std::endl;
	};
	//resumef::this_scheduler()->run_until_notask();

	go resumable_get_long(3);
	resumef::this_scheduler()->run_until_notask();
}
