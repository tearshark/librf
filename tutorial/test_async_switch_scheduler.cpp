
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

using namespace resumef;

static scheduler_t* sch_in_main = nullptr;
static std::atomic<scheduler_t*> sch_in_thread = nullptr;

void run_in_thread(channel_t<bool> c_done)
{
	local_scheduler my_scheduler;			//产生本线程唯一的调度器
	sch_in_thread = this_scheduler();		//本线程唯一的调度器赋值给sch_in_thread，以便于后续测试直接访问此线程的调度器

	(void)c_done.write(true);				//数据都准备好了，通过channel通知其他协程可以启动后续依赖sch_in_thread变量的协程了

	//循环直到sch_in_thread为nullptr
	for (;;)
	{
		auto sch = sch_in_thread.load(std::memory_order_acquire);
		if (sch == nullptr)
			break;
		sch->run_one_batch();
		std::this_thread::yield();
	}
}

template<class _Ctype>
static void callback_get_long_switch_scheduler(int64_t val, _Ctype&& cb)
{
	using namespace std::chrono;
	std::thread([val, cb = std::forward<_Ctype>(cb)]
		{
			std::this_thread::sleep_for(500ms);
			cb(val + 1);
		}).detach();
}

//这种情况下，没有生成 frame-context，因此，并没有promise_type被内嵌在frame-context里
static future_t<int64_t> async_get_long_switch_scheduler(int64_t val)
{
	awaitable_t<int64_t> awaitable;
	callback_get_long_switch_scheduler(val, [awaitable](int64_t result)
	{
		awaitable.set_value(result);
	});
	return awaitable.get_future();
}

//这种情况下，会生成对应的 frame-context，一个promise_type被内嵌在frame-context里
static future_t<> resumable_get_long_switch_scheduler(int64_t val, channel_t<bool> c_done)
{
	std::cout << "thread = " << std::this_thread::get_id();
	std::cout << ", scheduler = " << current_scheduler();
	std::cout << ", value = " << val << std::endl;

	co_await via(sch_in_thread);
	val = co_await async_get_long_switch_scheduler(val);

	std::cout << "thread = " << std::this_thread::get_id();
	std::cout << ", scheduler = " << current_scheduler();
	std::cout << ", value = " << val << std::endl;

	co_await via(sch_in_main);
	val = co_await async_get_long_switch_scheduler(val);

	std::cout << "thread = " << std::this_thread::get_id();
	std::cout << ", scheduler = " << current_scheduler();
	std::cout << ", value = " << val << std::endl;

	co_await via(sch_in_thread);
	val = co_await async_get_long_switch_scheduler(val);

	std::cout << "thread = " << std::this_thread::get_id();
	std::cout << ", scheduler = " << current_scheduler();
	std::cout << ", value = " << val << std::endl;

	co_await via(sch_in_thread);	//fake switch
	val = co_await async_get_long_switch_scheduler(val);

	std::cout << "thread = " << std::this_thread::get_id();
	std::cout << ", scheduler = " << current_scheduler();
	std::cout << ", value = " << val << std::endl;

	(void)c_done.write(true);
}

void resumable_main_switch_scheduler()
{
	sch_in_main = this_scheduler();

	std::cout << "main thread = " << std::this_thread::get_id();
	std::cout << ", scheduler = " << sch_in_main << std::endl;

	channel_t<bool> c_done{ 1 };
	std::thread other(&run_in_thread, std::ref(c_done));

	go[&other, c_done]()->future_t<>
	{
		co_await c_done;		//第一次等待，等待run_in_thread准备好了
		
		std::cout << "other thread = " << other.get_id();
		std::cout << ", sch_in_thread = " << sch_in_thread << std::endl;
	
		go resumable_get_long_switch_scheduler(1, c_done);	//开启另外一个协程
		//co_await resumable_get_long(3, c_done);
		co_await c_done;		//等待新的协程运行完毕，从而保证主线程的协程不会提早退出
	};

	sch_in_main->run_until_notask();

	//通知另外一个线程退出
	sch_in_thread.store(nullptr, std::memory_order_release);
	other.join();
}
