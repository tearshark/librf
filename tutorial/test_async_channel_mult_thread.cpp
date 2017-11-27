//验证channel是否线程安全

#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>
#include <deque>
#include <mutex>

#include "librf.h"

using namespace resumef;

using namespace std::chrono;
static std::mutex cout_mutex;

#define OUTPUT_DEBUG	0

future_vt test_channel_consumer(const channel_t<std::string> & c, size_t cnt)
{
	for (size_t i = 0; i < cnt; ++i)
	{
		try
		{
			auto val = co_await c.read();
#if OUTPUT_DEBUG
			{
				scoped_lock<std::mutex> __lock(cout_mutex);
				std::cout << "R " << val << "@" << std::this_thread::get_id() << std::endl;
			}
#endif
		}
		catch (channel_exception e)
		{
			//MAX_CHANNEL_QUEUE=0,并且先读后写，会触发read_before_write异常
			scoped_lock<std::mutex> __lock(cout_mutex);
			std::cout << e.what() << std::endl;
		}

#if OUTPUT_DEBUG
		co_await sleep_for(50ms);
#endif
	}
}

future_vt test_channel_producer(const channel_t<std::string> & c, size_t cnt)
{
	for (size_t i = 0; i < cnt; ++i)
	{
		co_await c.write(std::to_string(i));
#if OUTPUT_DEBUG
		{
			scoped_lock<std::mutex> __lock(cout_mutex);
			std::cout << "W " << i << "@" << std::this_thread::get_id() << std::endl;
		}
#endif
	}
}

const size_t N = 8;
const size_t BATCH = 1000000;
const size_t MAX_CHANNEL_QUEUE = N + 1;		//0, 1, 5, 10, -1

void resumable_main_channel_mult_thread()
{
	channel_t<std::string> c(MAX_CHANNEL_QUEUE);

	std::thread wth([&]
	{
		//local_scheduler my_scheduler;		//2017/11/27日，仍然存在BUG。真多线程下调度，存在有协程无法被调度完成的BUG
		go test_channel_producer(c, BATCH * N);
		this_scheduler()->run_until_notask();

		std::cout << "Write OK" << std::endl;
	});

	//std::this_thread::sleep_for(100ms);

	std::thread rth[N];
	for (size_t i = 0; i < N; ++i)
	{
		rth[i] = std::thread([&]
		{
			//local_scheduler my_scheduler;		//2017/11/27日，仍然存在BUG。真多线程下调度，存在有协程无法被调度完成的BUG
			go test_channel_consumer(c, BATCH);
			this_scheduler()->run_until_notask();

			std::cout << "Read OK" << std::endl;
		});
	}

	for(auto & th : rth)
		th.join();
	wth.join();

	std::cout << "OK" << std::endl;
	_getch();
}

//----------------------------------------------------------------------------------------------------------------------

/*
const size_t POOL_COUNT = 8;

//这是一个重度计算任务，只能单开线程来避免主线程被阻塞
auto async_heavy_computing_tasks(int64_t val)
{
	using namespace std::chrono;

	awaitable_t<int64_t> awaitable;

	std::thread([val, st = awaitable._state]
	{
		std::this_thread::sleep_for(500ms);
		st->set_value(val * val);
	}).detach();

	return awaitable;
}

future_vt heavy_computing_sequential(int64_t val)
{
	for (size_t i = 0; i < 3; ++i)
	{
		{
			scoped_lock<std::mutex> __lock(cout_mutex);
			std::cout << val << " @" << std::this_thread::get_id() << std::endl;
		}
		val = co_await async_heavy_computing_tasks(val);
	}
	{
		scoped_lock<std::mutex> __lock(cout_mutex);
		std::cout << val << " @" << std::this_thread::get_id() << std::endl;
	}
}

void test_use_single_thread(int64_t val)
{
	//使用local_scheduler来申明一个绑定到本线程的调度器 my_scheduler
	//后续在本线程运行的协程，通过this_scheduler()获得my_scheduler的地址
	//从而将这些协程的所有操作都绑定到my_scheduler里面去调度
	//实现一个协程始终绑定到一个线程的目的
	//在同一个线程里，申明多个local_scheduler会怎么样？
	//----我也不知道
	//如果不申明my_scheduler，则this_scheduler()获得默认主调度器的地址
	local_scheduler my_scheduler;

	{
		scoped_lock<std::mutex> __lock(cout_mutex);
		std::cout << "running in thread @" << std::this_thread::get_id() << std::endl;
	}
	go heavy_computing_sequential(val);

	this_scheduler()->run_until_notask();
}

const size_t N = 2;
void test_use_multi_thread()
{
	std::thread th_array[N];
	for (size_t i = 0; i < N; ++i)
		th_array[i] = std::thread(&test_use_single_thread, 4 + i);

	test_use_single_thread(3);

	for (auto & th : th_array)
		th.join();
}

void resumable_main_multi_thread()
{
	std::cout << "test_use_single_thread @" << std::this_thread::get_id() << std::endl << std::endl;
	test_use_single_thread(2);

	std::cout << std::endl;
	std::cout << "test_use_multi_thread @" << std::this_thread::get_id() << std::endl << std::endl;
	test_use_multi_thread();

	//运行主调度器里面的协程
	//但本范例不应该有协程存在，仅演示不要忽略了主调度器
	scheduler::g_scheduler.run_until_notask();
}

void resumable_main_channel_mult_thread()
{

}
*/