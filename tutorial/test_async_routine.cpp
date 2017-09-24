
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

using namespace resumef;

future_vt test_routine_use_timer()
{
	using namespace std::chrono;

	for (size_t i = 0; i < 10; ++i)
	{
		co_await resumef::sleep_for(100ms);
		std::cout << "timer after 100ms." << std::endl;
	}
}

future_vt test_routine_use_timer_2()
{
	co_await test_routine_use_timer();
	co_await test_routine_use_timer();
	co_await test_routine_use_timer();
}

void resumable_main_routine()
{
	go test_routine_use_timer_2();
	//test_routine_use_timer();
	g_scheduler.run_until_notask();
}
