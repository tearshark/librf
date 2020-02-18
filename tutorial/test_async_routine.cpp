
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

using namespace resumef;

future_t<> test_routine_use_timer()
{
	using namespace std::chrono;

	void* frame_ptr = _coro_frame_ptr();
	size_t frame_size = _coro_frame_size();
	std::cout << "test_routine_use_timer" << std::endl;
	std::cout << "frame point=" << frame_ptr << ", size=" << frame_size << ", promise_size=" << promise_align_size<>() << std::endl;

	auto handler = coroutine_handle<promise_t<>>::from_address(frame_ptr);
	auto st = handler.promise()._state;
	scheduler_t* sch = st->get_scheduler();
	auto parent = st->get_parent();
	std::cout << "st=" << st.get() << ", scheduler=" << sch << ", parent=" << parent << std::endl;

	for (size_t i = 0; i < 3; ++i)
	{
		co_await resumef::sleep_for(100ms);
		std::cout << "timer after 100ms" << std::endl;
		std::cout << "1:frame=" << _coro_frame_ptr() << std::endl;
	}
}

future_t<> test_routine_use_timer_2()
{
	std::cout << "test_routine_use_timer_2" << std::endl;

	co_await test_routine_use_timer();
	std::cout << "2:frame=" << _coro_frame_ptr() << std::endl;
	co_await test_routine_use_timer();
	std::cout << "2:frame=" << _coro_frame_ptr() << std::endl;
	co_await test_routine_use_timer();
	std::cout << "2:frame=" << _coro_frame_ptr() << std::endl;
}

void resumable_main_routine()
{
	//go test_routine_use_timer_2();
	go test_routine_use_timer();
	this_scheduler()->run_until_notask();
}
