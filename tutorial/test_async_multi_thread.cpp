#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "librf.h"

using namespace resumef;
static std::mutex cout_mutex;

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

	return awaitable.get_future();
}

future_t<> heavy_computing_sequential(int64_t val)
{
	for(size_t i = 0; i < 3; ++i)
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
	//使用local_scheduler_t来申明一个绑定到本线程的调度器 my_scheduler
	//后续在本线程运行的协程，通过this_scheduler()获得my_scheduler的地址
	//从而将这些协程的所有操作都绑定到my_scheduler里面去调度
	//实现一个协程始终绑定到一个线程的目的
	//在同一个线程里，申明多个local_scheduler_t会怎么样？
	//----我也不知道
	//如果不申明my_scheduler，则this_scheduler()获得默认主调度器的地址
	local_scheduler_t my_scheduler;
	
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
	scheduler_t::g_scheduler.run_until_notask();
}

int main()
{
	resumable_main_multi_thread();
	return 0;
}
