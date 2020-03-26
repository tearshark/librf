
#include "librf.h"
#include <optional>
#include <crtdbg.h>

//#define _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE	1
#include "src/ring_queue.h"
#include "src/ring_queue_spinlock.h"
#include "src/ring_queue_lockfree.h"
#include "../tutorial/test_ring_queue.h"

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
	//_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
	(void)argc;
	(void)argv;

	//test_ring_queue_simple<resumef::ring_queue<int>>();
	//test_ring_queue<resumef::ring_queue_spinlock<int, false, uint32_t>>();
	//test_ring_queue<resumef::ring_queue_lockfree<int, uint64_t>>();

	resumable_main_mutex();
	return 0;

	//if (argc > 1)
	//	resumable_main_benchmark_asio_client(atoi(argv[1]));
	//else
	//	resumable_main_benchmark_asio_server();

	resumable_main_cb(); _CrtCheckMemory();
	resumable_main_layout(); _CrtCheckMemory();
	resumable_main_modern_cb(); _CrtCheckMemory();
	resumable_main_suspend_always(); _CrtCheckMemory();
	resumable_main_yield_return(); _CrtCheckMemory();
	resumable_main_resumable(); _CrtCheckMemory();
	resumable_main_routine(); _CrtCheckMemory();
	resumable_main_exception(); _CrtCheckMemory();
	resumable_main_dynamic_go(); _CrtCheckMemory();
	resumable_main_multi_thread(); _CrtCheckMemory();
	resumable_main_timer(); _CrtCheckMemory();
	resumable_main_benchmark_mem(false); _CrtCheckMemory();
	resumable_main_mutex(); _CrtCheckMemory();
	resumable_main_event(); _CrtCheckMemory();
	resumable_main_event_v2(); _CrtCheckMemory();
	resumable_main_event_timeout(); _CrtCheckMemory();
	resumable_main_channel(); _CrtCheckMemory();
	resumable_main_channel_mult_thread(); _CrtCheckMemory();
	resumable_main_sleep(); _CrtCheckMemory();
	resumable_main_when_all(); _CrtCheckMemory();
	resumable_main_switch_scheduler(); _CrtCheckMemory();
	std::cout << "ALL OK!" << std::endl;

	benchmark_main_channel_passing_next();	//这是一个死循环测试
	_CrtCheckMemory();

	return 0;
}
