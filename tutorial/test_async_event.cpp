
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

using namespace resumef;

//非协程的逻辑线程，或异步代码，可以通过event_t通知到协程，并且不会阻塞协程所在的线程。
std::thread async_set_event(const event_t & e, std::chrono::milliseconds dt)
{
	return std::thread([=]
	{
		std::this_thread::sleep_for(dt);
		e.signal();
	});
}


future_t<> resumable_wait_event(const event_t & e)
{
	using namespace std::chrono;

	if (co_await e.wait() == false)
		std::cout << "time out!" << std::endl;
	else
		std::cout << "event signal!" << std::endl;
}

void test_wait_one()
{
	using namespace std::chrono;

	{
		event_t evt;
		go resumable_wait_event(evt);
		auto tt = async_set_event(evt, 1000ms);
		this_scheduler()->run_until_notask();

		tt.join();
	}
	{
		event_t evt2(1);
		go[&]() -> future_t<>
		{
			(void)co_await evt2.wait();
			std::cout << "event signal on 1!" << std::endl;
		};
		go[&]() -> future_t<>
		{
			(void)co_await evt2.wait();
			std::cout << "event signal on 2!" << std::endl;
		};
		std::cout << std::this_thread::get_id() << std::endl;
		auto tt = async_set_event(evt2, 1000ms);

		this_scheduler()->run_until_notask();

		tt.join();
	}
}

void test_wait_any()
{
	using namespace std::chrono;

	event_t evts[8];

	go[&]() -> future_t<>
	{
		for (int i = 0; i < _countof(evts); ++i)
		{
			intptr_t idx = co_await event_t::wait_any(evts);
			std::cout << "event " << idx << " signal!" << std::endl;
		}
	};

	std::vector<std::thread> vtt;

	srand((int)time(nullptr));
	for (auto & e : evts)
	{
		vtt.emplace_back(async_set_event(e, 1ms * (500 + rand() % 1000)));
	}

	this_scheduler()->run_until_notask();

	for (auto & tt : vtt)
		tt.join();
}

void test_wait_all()
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

	std::vector<std::thread> vtt;

	srand((int)time(nullptr));
	for (auto & e : evts)
	{
		vtt.emplace_back(async_set_event(e, 1ms * (500 + rand() % 1000)));
	}

	this_scheduler()->run_until_notask();

	for (auto & tt : vtt)
		tt.join();
}

void test_wait_all_timeout()
{
	using namespace std::chrono;

	event_t evts[8];

	go[&]() -> future_t<>
	{
		if (co_await event_t::wait_all_for(1000ms, evts))
			std::cout << "all event signal!" << std::endl;
		else
			std::cout << "time out!" << std::endl;
	};

	std::vector<std::thread> vtt;

	srand((int)time(nullptr));
	for (auto & e : evts)
	{
		vtt.emplace_back(async_set_event(e, 1ms * (500 + rand() % 1000)));
	}

	this_scheduler()->run_until_notask();

	for (auto & tt : vtt)
		tt.join();
}

void resumable_main_event()
{
	test_wait_one();
	std::cout << std::endl;

	test_wait_any();
	std::cout << std::endl;

	test_wait_all();
	std::cout << std::endl;

	test_wait_all_timeout();
	std::cout << std::endl;
}
