#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "librf.h"

using namespace resumef;
using namespace std::chrono;

//token触发停止后，将不再调用cb
template<class _Ctype>
static void callback_get_long_with_stop(stop_token token, int64_t val, _Ctype&& cb)
{
	std::thread([val, token = std::move(token), cb = std::forward<_Ctype>(cb)]
		{
			for (int i = 0; i < 10; ++i)
			{
				if (token.stop_requested())
					return;
				std::this_thread::sleep_for(10ms);
			}

			cb(val * val);
		}).detach();
}

//token触发后，设置canceled_exception异常。
static future_t<int64_t> async_get_long_with_stop(stop_token token, int64_t val)
{
	awaitable_t<int64_t> awaitable;
	
	//保证stopptr的生存期，与callback_get_long_with_cancel()的回调参数的生存期一致。
	//如果token已经被取消，则传入的lambda会立即被调用，则awaitable将不能再set_value
	auto stopptr = make_stop_callback(token, [awaitable]
	{
		if (awaitable)
			awaitable.throw_exception(canceled_exception(error_code::stop_requested));
	});

	if (awaitable)	//处理已经被取消的情况
	{
		callback_get_long_with_stop(token, val, [awaitable, stopptr = std::move(stopptr)](int64_t val)
		{
			if (awaitable)
				awaitable.set_value(val);
		});
	}

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
}
