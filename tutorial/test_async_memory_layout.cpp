#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "librf/librf.h"

using namespace librf;

#ifndef __GNUC__	//GCC: 没有提供__builtin_coro_frame这样的内置函数

template<class _Ctype>
static void callback_get_long(int64_t a, int64_t b, _Ctype&& cb)
{
	std::cout << std::endl << __FUNCTION__ << " - begin" << std::endl;

	//编译失败。因为这个函数不是"可恢复函数(resumeable function)"，甚至都不是"可等待函数(awaitable function)"
	//void* frame_ptr = _coro_frame_ptr();

	using namespace std::chrono;
	std::thread([=, cb = std::forward<_Ctype>(cb)]
		{
			std::this_thread::sleep_for(500ms);
			cb(a + b);
		}).detach();

	std::cout << __FUNCTION__ << " - end" << std::endl;
}

//这种情况下，没有生成 frame-context，因此，并没有promise_type被内嵌在frame-context里
future_t<int64_t> awaitable_get_long(int64_t a, int64_t b)
{
	std::cout << std::endl << __FUNCTION__ << " - begin" << std::endl;
	//编译失败。因为这个函数不是"可恢复函数(resumeable function)"，仅仅是"可等待函数(awaitable function)"
	//void* frame_ptr = _coro_frame_ptr();

	librf::awaitable_t<int64_t> awaitable;
	callback_get_long(a, b, [awaitable](int64_t val)
	{
		awaitable.set_value(val);
	});

	std::cout << __FUNCTION__ << " - end" << std::endl;

	return awaitable.get_future();
}

future_t<int64_t> resumeable_get_long(int64_t x, int64_t y)
{
	std::cout << std::endl << __FUNCTION__ << " - begin" << std::endl;

	using future_type = future_t<int64_t>;
	using promise_type = typename future_type::promise_type;
	using state_type = typename future_type::state_type;

	void* frame_ptr = _coro_frame_ptr();
	auto handler = coroutine_handle<promise_type>::from_address(frame_ptr);
	promise_type* promise = &handler.promise();
	state_type* state = handler.promise().get_state();

	std::cout << "  future size=" << sizeof(future_type) << " / " << _Align_size<future_type>() << std::endl;
	std::cout << "  promise size=" << sizeof(promise_type) << " / " << _Align_size<promise_type>() << std::endl;
	std::cout << "  state size=" << sizeof(state_type) << " / "<< _Align_size<state_type>() << std::endl;
	std::cout << "  frame size=" << _coro_frame_size() << ", alloc size=" << state->get_alloc_size() << std::endl;

	std::cout << "  frame ptr=" << frame_ptr << "," << (void*)&frame_ptr << std::endl;
	std::cout << "  frame end=" << (void*)((char*)(frame_ptr)+_coro_frame_size()) << std::endl;
	std::cout << "  promise ptr=" << promise << "," << (void*)&promise << std::endl;
	std::cout << "  handle ptr=" << handler.address() << "," << (void*)&handler << std::endl;
	std::cout << "  state ptr=" << state << "," << (void*)&state << std::endl;
	std::cout << "  parent ptr=" << state->get_parent() << std::endl;

	std::cout << "    x=" << x << ", &x=" << std::addressof(x) << std::endl;
	std::cout << "    y=" << y << ", &y=" << std::addressof(y) << std::endl;

	int64_t val = co_await awaitable_get_long(x, y);
	std::cout << "    val=" << val << ", &val=" << std::addressof(val) << std::endl;

	std::cout << __FUNCTION__ << " - end" << std::endl;

	co_return val;
}

//这种情况下，会生成对应的 frame-context，一个promise_type被内嵌在frame-context里
future_t<> resumable_get_long_2(int64_t a, int64_t b, int64_t c)
{
	int64_t v1, v2, v3;

	std::cout << std::endl << __FUNCTION__ << " - begin" << std::endl;

	using future_type = future_t<>;
	using promise_type = typename future_type::promise_type;
	using state_type = typename future_type::state_type;

	void* frame_ptr = _coro_frame_ptr();
	auto handler = coroutine_handle<promise_type>::from_address(frame_ptr);
	promise_type * promise = &handler.promise();
	state_type * state = handler.promise().get_state();

	std::cout << "  future size=" << sizeof(future_type) << " / " << _Align_size<future_type>() << std::endl;
	std::cout << "  promise size=" << sizeof(promise_type) << " / " << _Align_size<promise_type>() << std::endl;
	std::cout << "  state size=" << sizeof(state_type) << " / "<< _Align_size<state_type>() << std::endl;
	std::cout << "  frame size=" << _coro_frame_size() << ", alloc size=" << state->get_alloc_size() << std::endl;

	std::cout << "  frame ptr=" << frame_ptr << ","<< (void*)&frame_ptr << std::endl;
	std::cout << "  frame end=" << (void *)((char*)(frame_ptr) + _coro_frame_size()) << std::endl;
	std::cout << "  promise ptr=" << promise << "," << (void *)&promise << std::endl;
	std::cout << "  handle ptr=" << handler.address() << "," << (void*)&handler << std::endl;
	std::cout << "  state ptr=" << state << "," << (void*)&state << std::endl;
	std::cout << "  parent ptr=" << state->get_parent() << std::endl;

	std::cout << "    a=" << a << ", &a=" << std::addressof(a) << std::endl;
	std::cout << "    b=" << b << ", &b=" << std::addressof(b) << std::endl;
	std::cout << "    c=" << c << ", &c=" << std::addressof(c) << std::endl;

	v1 = co_await resumeable_get_long(a, b);
	std::cout << "    v1=" << v1 << ", &v1=" << std::addressof(v1) << std::endl;

	v2 = co_await resumeable_get_long(b, c);
	std::cout << "    v2=" << v2 << ", &v2=" << std::addressof(v2) << std::endl;

	v3 = co_await resumeable_get_long(v1, v2);
	std::cout << "    v3=" << v3 << ", &v3=" << std::addressof(v3) << std::endl;

	int64_t v4 = v1 * v2 * v3;
	std::cout << "    v4=" << v4 << ", &v4=" << std::addressof(v4) << std::endl;

	std::cout << __FUNCTION__ << " - end" << std::endl;
}
#endif //#ifndef __GNUC__

void resumable_main_layout()
{
	std::cout << std::endl << __FUNCTION__ << " - begin" << std::endl;

#ifndef __GNUC__	//GCC: 没有提供__builtin_coro_frame这样的内置函数
	go resumable_get_long_2(1, 2, 5);
#endif //#ifndef __GNUC__
	librf::this_scheduler()->run_until_notask();

	std::cout << __FUNCTION__ << " - end" << std::endl;
}

#if LIBRF_TUTORIAL_STAND_ALONE
int main()
{
	resumable_main_layout();
	return 0;
}
#endif
