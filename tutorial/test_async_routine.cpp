
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
