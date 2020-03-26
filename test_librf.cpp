
#include "librf/librf.h"
#include <iostream>

extern void resumable_main_yield_return();
extern void resumable_main_timer();
extern void resumable_main_suspend_always();
extern void resumable_main_sleep();
extern void resumable_main_routine();
extern void resumable_main_resumable();
extern void resumable_main_mutex();
extern void resumable_main_exception();
extern void resumable_main_event();
extern void resumable_main_event_v2();
extern void resumable_main_event_timeout();
extern void resumable_main_dynamic_go();
extern void resumable_main_channel();
extern void resumable_main_cb();
extern void resumable_main_modern_cb();
extern void resumable_main_multi_thread();
extern void resumable_main_channel_mult_thread();
extern void resumable_main_when_all();
extern void resumable_main_layout();
extern void resumable_main_switch_scheduler();

extern void resumable_main_benchmark_mem(bool wait_key);
extern void benchmark_main_channel_passing_next();
extern void resumable_main_benchmark_asio_server();
extern void resumable_main_benchmark_asio_client(intptr_t nNum);

int main(int argc, const char* argv[])
{
    std::cout << __clang_major__ << std::endl;
    
	(void)argc;
	(void)argv;
	return 0;
}
