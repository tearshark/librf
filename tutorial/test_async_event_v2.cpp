#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "librf/librf.h"

using namespace librf;
using namespace std::chrono;

//非协程的逻辑线程，或异步代码，可以通过event_t通知到协程，并且不会阻塞协程所在的线程。
static std::thread async_set_event_all(const event_t & e, std::chrono::milliseconds dt)
{
	return std::thread([=]
	{
		std::this_thread::sleep_for(dt);
		e.signal_all();
	});
}

static std::thread async_set_event_one(event_t e, std::chrono::milliseconds dt)
{
	return std::thread([=]
	{
		std::this_thread::sleep_for(dt);
		e.signal();
	});
}

static future_t<> resumable_wait_event(event_t e, int idx)
{
	auto result = co_await e;
	if (result)
		std::cout << "[" << idx << "]event signal!" << std::endl;
	else
		std::cout << "[" << idx << "]time out!" << std::endl;
}

static future_t<> resumable_wait_timeout(event_t e, milliseconds dt, int idx)
{
	auto result = co_await e.wait_for(dt);
	if (result)
		std::cout << "[" << idx << "]event signal!" << std::endl;
	else
		std::cout << "[" << idx << "]time out!" << std::endl;
}

static void test_notify_all()
{
	event_t evt;
	go resumable_wait_event(evt, 0);
	go resumable_wait_event(evt, 1);
	go resumable_wait_event(evt, 2);

	auto tt = async_set_event_all(evt, 100ms);
	this_scheduler()->run_until_notask();

	tt.join();
}

static void test_notify_one()
{
	using namespace std::chrono;

	{
		event_t evt;
		go resumable_wait_event(evt, 10);
		go resumable_wait_event(evt, 11);
		go resumable_wait_event(evt, 12);

		auto tt1 = async_set_event_one(evt, 100ms);
		auto tt2 = async_set_event_one(evt, 500ms);
		auto tt3 = async_set_event_one(evt, 800ms);
		this_scheduler()->run_until_notask();

		tt1.join();
		tt2.join();
		tt3.join();
	}
}

static void test_wait_all_timeout()
{
	using namespace std::chrono;

	srand((int)time(nullptr));

	event_t evts[10];

	std::vector<std::thread> vtt;
	for(size_t i = 0; i < std::size(evts); ++i)
	{
		go resumable_wait_timeout(evts[i], 100ms, (int)i);
		vtt.emplace_back(async_set_event_one(evts[i], 1ms * (50 + i * 10)));
	}

	this_scheduler()->run_until_notask();

	for (auto& tt : vtt)
		tt.join();
}

void resumable_main_event_v2()
{
	test_notify_all();
	std::cout << std::endl;

	test_notify_one();
	std::cout << std::endl;

	test_wait_all_timeout();
	std::cout << std::endl;
}

int main()
{
	resumable_main_event_v2();
	return 0;
}
