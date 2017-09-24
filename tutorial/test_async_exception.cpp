
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

using namespace resumef;

//请打开结构化异常(/EHa)
auto async_signal_exception(const intptr_t dividend)
{
	using namespace std::chrono;

	resumef::awaitable_t<int64_t> awaitable;

	std::thread([dividend, st = awaitable._state]
	{
		std::this_thread::sleep_for(50ms);
		try
		{
			if (dividend == 0)
				throw std::logic_error("divided by zero");
			st->set_value(10000 / dividend);
		}
		catch (...)
		{
			st->set_exception(std::current_exception());
		}
	}).detach();

	return awaitable;
}

future_vt test_signal_exception()
{
	for (intptr_t i = 10; i >= 0; --i)
	{
		try
		{
			auto r = co_await async_signal_exception(i);
			std::cout << "result is " << r << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cout << "exception signal : " << e.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "exception signal : who knows?" << std::endl;
		}
	}
}

future_vt test_bomb_exception()
{
	for (intptr_t i = 10; i >= 0; --i)
	{
		auto r = co_await async_signal_exception(i);
		std::cout << "result is " << r << std::endl;
	}
}

void resumable_main_exception()
{
	go test_signal_exception();
	g_scheduler.run_until_notask();

	std::cout << std::endl;

	go test_bomb_exception();
	g_scheduler.run_until_notask();
}
