#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "librf.h"

using namespace resumef;
using namespace std::chrono;

//_Ctype签名:void(bool, int64_t)
template<class _Ctype, typename=std::enable_if_t<std::is_invocable_v<_Ctype, bool, int64_t>>>
static void callback_get_long_with_stop(stop_token token, int64_t val, _Ctype&& cb)
{
	std::thread([val, token = std::move(token), cb = std::forward<_Ctype>(cb)]
		{
			for (int i = 0; i < 10; ++i)
			{
				if (token.stop_requested())
				{
					cb(false, 0);
					return;
				}
				std::this_thread::sleep_for(10ms);
			}

			//有可能未检测到token的停止要求
			//如果使用stop_callback来停止，则务必保证检测到的退出要求是唯一的，且线程安全的
			//否则，多次调用cb，会导致协程在半退出状态下，外部的awaitable_t管理的state获取跟root出现错误。
			cb(true, val * val);
		}).detach();
}

//token触发后，设置canceled_exception异常。
static future_t<int64_t> async_get_long_with_stop(stop_token token, int64_t val)
{
	awaitable_t<int64_t> awaitable;

	//在这里通过stop_callback来处理退出，并将退出转化为error_code::stop_requested异常。
	//则必然会存在线程竞争问题，导致协程提前于callback_get_long_with_stop的回调之前而退出。
	//同时，callback_get_long_with_stop还未必一定能检测到退出要求----毕竟只是一个要求，而不是强制。

	callback_get_long_with_stop(token, val, [awaitable](bool ok, int64_t val)
	{
		if (ok)
			awaitable.set_value(val);
		else
			awaitable.throw_exception(canceled_exception{error_code::stop_requested});
	});
	return awaitable.get_future();
}

//如果关联的协程被取消了，则触发canceled_exception异常。
static future_t<int64_t> async_get_long_with_stop(int64_t val)
{
	task_t* task = current_task();
	co_return co_await async_get_long_with_stop(task->get_stop_token(), val);
}

//测试取消协程
static void test_get_long_with_stop(int64_t val)
{
	//异步获取值的协程
	task_t* task = GO
	{
		try
		{
			int64_t result = co_await async_get_long_with_stop(val);
			std::cout << result << std::endl;
		}
		catch (const std::logic_error& e)
		{
			std::cout << e.what() << std::endl;
		}
	};
	//task的生命周期只在task代表的协程生存期间存在。
	//但通过复制与其关联的stop_source，生存期可以超过task的生存期。
	stop_source stops = task->get_stop_source();

	//取消上一个协程的延迟协程
	GO
	{
		co_await sleep_for(1ms * (rand() % 300));
		stops.request_stop();
	};

	this_scheduler()->run_until_notask();
}

void resumable_main_stop_token()
{
	srand((int)time(nullptr));
	for (int i = 0; i < 10; ++i)
		test_get_long_with_stop(i);

	std::cout << "OK - stop_token!" << std::endl;
}
