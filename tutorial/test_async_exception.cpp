#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "librf.h"

using namespace resumef;

//请打开结构化异常(/EHa)
auto async_signal_exception(const intptr_t dividend)
{
	awaitable_t<int64_t> awaitable;

	std::thread([dividend, awaitable]
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		try
		{
			//也可以注释掉这个判断，使用结构化异常。但就获得不了具体描述信息了
			if (dividend == 0)
				throw std::logic_error("divided by zero");
			awaitable.set_value(10000 / dividend);
		}
		catch (...)
		{
			awaitable.set_exception(std::current_exception());
		}
	}).detach();

	return awaitable.get_future();
}

auto async_signal_exception2(const intptr_t dividend)
{
	awaitable_t<int64_t> awaitable;

	std::thread([dividend, awaitable]
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		if (dividend == 0)
			awaitable.throw_exception(std::logic_error("divided by zero"));
		else
			awaitable.set_value(10000 / dividend);
	}).detach();

	return awaitable.get_future();
}

future_t<> test_signal_exception()
{
	for (intptr_t i = 10; i >= 0; --i)
	{
		try
		{
			auto r = co_await async_signal_exception2(i);
			std::cout << "result is " << r << std::endl;
		}
		catch (const std::exception& ex)
		{
			std::cout << "exception signal : " << ex.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "exception signal : who knows?" << std::endl;
		}
	}
}

future_t<> test_bomb_exception()
{
	for (intptr_t i = 10; i >= 0; --i)
	{
		auto r = co_await async_signal_exception(i);
		std::cout << "result is " << r << std::endl;
	}
}

void resumable_main_exception(bool bomb)
{
	std::cout << __FUNCTION__ << std::endl;
	go test_signal_exception();
	this_scheduler()->run_until_notask();

	std::cout << std::endl;
	if (bomb)
	{
		go test_bomb_exception();
		this_scheduler()->run_until_notask();
	}
}

int main()
{
	resumable_main_exception(true);
	return 0;
}
