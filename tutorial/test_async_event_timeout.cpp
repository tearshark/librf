
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

using namespace resumef;

future_t<> resumalbe_set_event(const event_t & e, std::chrono::milliseconds dt)
{
	co_await resumef::sleep_for(dt);
	e.signal();
	std::cout << "+";
}

void async_set_event(const event_t & e, std::chrono::milliseconds dt)
{
	std::thread([=]
	{
		std::this_thread::sleep_for(dt);
		e.signal();
	}).detach();
}

void test_wait_timeout_one()
{
	std::cout << __FUNCTION__ << std::endl;
	using namespace std::chrono;

	event_t evt;

	go [&evt]() -> future_t<>
	{
		intptr_t counter = 0;
		for (;;)
		{
			if (co_await evt.wait_for(500ms))
				break;
			++counter;
			std::cout << ".";
		}
		std::cout << counter << std::endl;
	};

	async_set_event(evt, 10s + 50ms);
	//go resumalbe_set_event(evt, 10s + 50ms);

	this_scheduler()->run_until_notask();
}

void test_wait_timeout_any_invalid()
{
	std::cout << __FUNCTION__ << std::endl;
	using namespace std::chrono;

	event_t evts[8];

	//无效的等待
	go[&]()-> future_t<>
	{
		intptr_t idx = co_await event_t::wait_any_for(500ms, std::begin(evts), std::end(evts));
		assert(idx < 0);
		(void)idx;

		std::cout << "invalid wait!" << std::endl;
	};
	this_scheduler()->run_until_notask();
}

void test_wait_timeout_any()
{
	std::cout << __FUNCTION__ << std::endl;
	using namespace std::chrono;

	event_t evts[8];

	go[&]() -> future_t<>
	{
		intptr_t counter = 0;
		for (;;)
		{
			intptr_t idx = co_await event_t::wait_any_for(500ms, evts);
			if (idx >= 0)
			{
				std::cout << counter << std::endl;
				std::cout << "event " << idx << " signal!" << std::endl;
				break;
			}

			++counter;
			std::cout << ".";
		}

		//取消剩下的定时器，以便于协程调度器退出来
		this_scheduler()->timer()->clear();
	};

	srand((int)time(nullptr));
	for (auto & e : evts)
	{
		//go resumalbe_set_event(e, 1ms * (1000 + rand() % 5000));
		async_set_event(e, 1ms * (1000 + rand() % 5000));
	}

	this_scheduler()->run_until_notask();
}

void test_wait_timeout_all_invalid()
{
	std::cout << __FUNCTION__ << std::endl;
	using namespace std::chrono;

	event_t evts[8];

	//无效的等待
	go[&]()-> future_t<>
	{
		bool result = co_await event_t::wait_all_for(500ms, std::begin(evts), std::end(evts));
		assert(!result);
		(void)result;

		std::cout << "invalid wait!" << std::endl;
	};
	this_scheduler()->run_until_notask();
}

void test_wait_timeout_all()
{
	std::cout << __FUNCTION__ << std::endl;
	using namespace std::chrono;

	event_t evts[8];

	go[&]() -> future_t<>
	{
		intptr_t counter = 0;
		for (;;)
		{
			if (co_await event_t::wait_all_for(500ms, evts))
			{
				std::cout << counter << std::endl;
				std::cout << "all event signal!" << std::endl;
				break;
			}

			++counter;
			std::cout << ".";
		}
	};

	srand((int)time(nullptr));
	for (auto & e : evts)
	{
		go resumalbe_set_event(e, 1ms * (1000 + rand() % 5000));
		//async_set_event(e, 1ms * (1000 + rand() % 5000));
	}

	this_scheduler()->run_until_notask();
}

void resumable_main_event_timeout()
{
	using namespace std::chrono;

	test_wait_timeout_one();
	std::cout << std::endl;

	test_wait_timeout_any_invalid();
	std::cout << std::endl << std::endl;

	test_wait_timeout_any();
	std::cout << std::endl << std::endl;

	test_wait_timeout_all_invalid();
	std::cout << std::endl;

	test_wait_timeout_all();
	std::cout << std::endl;
}
