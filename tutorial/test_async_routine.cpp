
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "librf.h"

using namespace resumef;

#ifndef __GNUC__	//GCC: 没有提供__builtin_coro_frame这样的内置函数
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
#endif //#ifndef __GNUC__

void resumable_main_routine()
{
	std::cout << __FUNCTION__ << std::endl;
	//go test_routine_use_timer_2();
#ifndef __GNUC__	//GCC: 没有提供__builtin_coro_frame这样的内置函数
	go test_routine_use_timer();
#endif //#ifndef __GNUC__
	this_scheduler()->run_until_notask();
}
