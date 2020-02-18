
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

using namespace resumef;

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
future_t<int64_t> async_get_long(int64_t val)
{
/*
	void* frame_ptr = _coro_frame_ptr();
	size_t frame_size = _coro_frame_size();
	std::cout << "test_routine_use_timer" << std::endl;
	std::cout << "frame point=" << frame_ptr << ", size=" << frame_size << ", promise_size=" << promise_align_size<>() << std::endl;

	auto handler = coroutine_handle<promise_t<>>::from_address(frame_ptr);
	auto st = handler.promise()._state;
	scheduler_t* sch = st->get_scheduler();
	auto parent = st->get_parent();
	std::cout << "st=" << st.get() << ", scheduler=" << sch << ", parent=" << parent << std::endl;
*/

	resumef::awaitable_t<int64_t> awaitable;
	callback_get_long(val, [awaitable](int64_t val)
	{
		awaitable.set_value(val);
	});
	return awaitable.get_future();
}

future_t<> wait_get_long(int64_t val)
{
	co_await async_get_long(val);
}

//这种情况下，会生成对应的 frame-context，一个promise_type被内嵌在frame-context里
future_t<> resumable_get_long(int64_t val)
{
	std::cout << val << std::endl;
	val = co_await async_get_long(val);
	std::cout << val << std::endl;
	val = co_await async_get_long(val);
	std::cout << val << std::endl;
	val = co_await async_get_long(val);
	std::cout << val << std::endl;
}

future_t<int64_t> loop_get_long(int64_t val)
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
	std::cout << std::this_thread::get_id() << std::endl;

	GO
	{
		auto val = co_await loop_get_long(2);
		std::cout << "GO:" << val << std::endl;
	};

	go resumable_get_long(3);
	resumef::this_scheduler()->run_until_notask();
}
