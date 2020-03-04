
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

using namespace resumef;

//非协程的逻辑线程，或异步代码，可以通过event_t通知到协程，并且不会阻塞协程所在的线程。
std::thread async_set_event_all(const v2::event_t & e, std::chrono::milliseconds dt)
{
	return std::thread([=]
	{
		std::this_thread::sleep_for(dt);
		e.notify_all();
	});
}

std::thread async_set_event_one(const v2::event_t& e, std::chrono::milliseconds dt)
{
	return std::thread([=]
	{
		std::this_thread::sleep_for(dt);
		e.notify_one();
	});
}


future_t<> resumable_wait_event(const v2::event_t & e, int idx)
{
	co_await e;
	std::cout << "[" << idx << "]event signal!" << std::endl;
}

void test_notify_all()
{
	using namespace std::chrono;

	{
		v2::event_t evt;
		go resumable_wait_event(evt, 0);
		go resumable_wait_event(evt, 1);
		go resumable_wait_event(evt, 2);

		auto tt = async_set_event_all(evt, 100ms);
		this_scheduler()->run_until_notask();

		tt.join();
	}
}

//目前还没法测试在多线程调度下，是否线程安全
void test_notify_one()
{
	using namespace std::chrono;

	{
		v2::event_t evt;
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

void resumable_main_event_v2()
{
	test_notify_all();
	std::cout << std::endl;

	test_notify_one();
	std::cout << std::endl;
}
