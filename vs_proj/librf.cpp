
#include "librf.h"
#include <experimental/resumable>
#include <experimental/generator>
#include <optional>

extern void resumable_main_yield_return();
extern void resumable_main_timer();
extern void resumable_main_suspend_always();
extern void resumable_main_sleep();
extern void resumable_main_routine();
extern void resumable_main_resumable();
extern void resumable_main_mutex();
extern void resumable_main_exception();
extern void resumable_main_event();
extern void resumable_main_event_timeout();
extern void resumable_main_dynamic_go();
extern void resumable_main_channel();
extern void resumable_main_cb();
extern void resumable_main_modern_cb();
extern void resumable_main_multi_thread();
extern void resumable_main_channel_mult_thread();
extern void resumable_main_when_all();

extern void resumable_main_benchmark_mem();
extern void resumable_main_benchmark_asio_server();
extern void resumable_main_benchmark_asio_client(intptr_t nNum);

void async_get_long(int64_t val, std::function<void(int64_t)> cb)
{
	using namespace std::chrono;
	std::thread([val, cb = std::move(cb)]
		{
			std::this_thread::sleep_for(10s);
			cb(val * val);
		}).detach();
}

//这种情况下，没有生成 frame-context，因此，并没有promise_type被内嵌在frame-context里
resumef::future_t<int64_t> co_get_long(int64_t val)
{
	resumef::awaitable_t<int64_t > st;
	std::cout << "co_get_long@1" << std::endl;
	
	async_get_long(val, [st](int64_t value)
	{
		std::cout << "co_get_long@2" << std::endl;
		st.set_value(value);
	});

	std::cout << "co_get_long@3" << std::endl;
	return st.get_future();
}

//这种情况下，会生成对应的 frame-context，一个promise_type被内嵌在frame-context里
resumef::future_t<> test_librf2()
{
	auto f = co_await co_get_long(2);
	std::cout << f << std::endl;
}

int main(int argc, const char* argv[])
{
	resumable_main_cb();

/*
	resumable_main_resumable();
	resumable_main_benchmark_mem();
	if (argc > 1)
		resumable_main_benchmark_asio_client(atoi(argv[1]));
	else
		resumable_main_benchmark_asio_server();

	resumable_main_when_all();
	resumable_main_multi_thread();
	resumable_main_yield_return();
	resumable_main_timer();
	resumable_main_suspend_always();
	resumable_main_sleep();
	resumable_main_routine();
	resumable_main_resumable();
	resumable_main_mutex();
	resumable_main_event();
	resumable_main_event_timeout();
	resumable_main_dynamic_go();
	resumable_main_channel();
	resumable_main_cb();
	resumable_main_exception();
*/

	return 0;
}
