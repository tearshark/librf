
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "librf.h"

using namespace resumef;

future_t<> test_sleep_use_timer()
{
	using namespace std::chrono;

	(void)sleep_for(100ms);		//incorrect!!!

	co_await sleep_for(100ms);
	std::cout << "sleep_for 100ms." << std::endl;
	co_await 100ms;
	std::cout << "co_await 100ms." << std::endl;

	try
	{
		co_await sleep_until(system_clock::now() + 200ms);
		std::cout << "timer after 200ms." << std::endl;
	}
	catch (timer_canceled_exception)
	{
		std::cout << "timer canceled." << std::endl;
	}
}

void test_wait_all_events_with_signal_by_sleep()
{
	using namespace std::chrono;

	event_t evts[8];

	go[&]() -> future_t<>
	{
		if (co_await event_t::wait_all(evts))
			std::cout << "all event signal!" << std::endl;
		else
			std::cout << "time out!" << std::endl;
	};

	srand((int)time(nullptr));
	for (size_t i = 0; i < std::size(evts); ++i)
	{
		go[&, i]() -> future_t<>
		{
			co_await sleep_for(1ms * (500 + rand() % 1000));
			evts[i].signal();
			std::cout << "event[ " << i << " ] signal!" << std::endl;
		};
	}

	while (!this_scheduler()->empty())
	{
		this_scheduler()->run_one_batch();
		//std::cout << "press any key to continue." << std::endl;
		//_getch();
	}
}

void resumable_main_sleep()
{
	go test_sleep_use_timer();
	this_scheduler()->run_until_notask();
	std::cout << std::endl;

	test_wait_all_events_with_signal_by_sleep();
	std::cout << std::endl;
}
