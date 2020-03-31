#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <deque>
#include <mutex>

#include "librf.h"

using namespace resumef;
using namespace std::chrono;

const size_t MAX_CHANNEL_QUEUE = 1;		//0, 1, 5, 10, -1

//如果使用move_only_type来操作channel失败，说明中间过程发生了拷贝操作----这不是设计目标。
template<class _Ty>
struct move_only_type
{
	_Ty value;

	move_only_type() = default;
	explicit move_only_type(const _Ty& val) : value(val) {}
	explicit move_only_type(_Ty&& val) : value(std::forward<_Ty>(val)) {}

	move_only_type(const move_only_type&) = delete;
	move_only_type& operator =(const move_only_type&) = delete;

	move_only_type(move_only_type&&) = default;
	move_only_type& operator =(move_only_type&&) = default;
};

//如果channel缓存的元素不能凭空产生，或者产生代价较大，则推荐第二个模板参数使用true。从而减小不必要的开销。
using string_channel_t = channel_t<move_only_type<std::string>>;

//channel其实内部引用了一个channel实现体，故可以支持复制拷贝操作
future_t<> test_channel_read(string_channel_t c)
{
	using namespace std::chrono;

	for (size_t i = 0; i < 10; ++i)
	{
#ifndef __clang__
		try
#endif
		{
			//auto val = co_await c.read();
			auto val = co_await c;		//第二种从channel读出数据的方法。利用重载operator co_await()，而不是c是一个awaitable_t。

			std::cout << val.value << ":";
			std::cout << std::endl;
		}
#ifndef __clang__
		catch (resumef::channel_exception& e)
		{
			//MAX_CHANNEL_QUEUE=0,并且先读后写，会触发read_before_write异常
			std::cout << e.what() << std::endl;
		}
#endif

		co_await sleep_for(50ms);
	}
}

future_t<> test_channel_write(string_channel_t c)
{
	using namespace std::chrono;

	for (size_t i = 0; i < 10; ++i)
	{
		//co_await c.write(std::to_string(i));
		co_await(c << std::to_string(i));				//第二种写入数据到channel的方法。因为优先级关系，需要将'c << i'括起来
		std::cout << "<" << i << ">:";
		
		std::cout << std::endl;
	}
}

void test_channel_read_first()
{
	string_channel_t c(MAX_CHANNEL_QUEUE);

	go test_channel_read(c);
	go test_channel_write(c);

	this_scheduler()->run_until_notask();
}

void test_channel_write_first()
{
	string_channel_t c(MAX_CHANNEL_QUEUE);

	go test_channel_write(c);
	go test_channel_read(c);

	this_scheduler()->run_until_notask();
}

static const int N = 1000000;

void test_channel_performance_single_thread(size_t buff_size)
{
	//1的话，效率跟golang比，有点惨不忍睹。
	//1000的话，由于几乎不需要调度器接入，效率就很高了，随便过千万数量级。
	channel_t<int, false, true> c{ buff_size };

	go[&]() -> future_t<>
	{
		for (int i = N - 1; i >= 0; --i)
		{
			co_await(c << i);
		}
	};
	go[&]() -> future_t<>
	{
		auto tstart = high_resolution_clock::now();

		int i;
		do
		{
			i = co_await c;
		} while (i > 0);

		auto dt = duration_cast<duration<double>>(high_resolution_clock::now() - tstart).count();
		std::cout << "channel buff=" << c.capacity() << ", w/r " << N << " times, cost time " << dt << "s" << std::endl;
	};

	this_scheduler()->run_until_notask();
}

void test_channel_performance_double_thread(size_t buff_size)
{
	//1的话，效率跟golang比，有点惨不忍睹。
	//1000的话，由于几乎不需要调度器接入，效率就很高了，随便过千万数量级。
	channel_t<int, false, true> c{ buff_size };

	std::thread wr_th([c]
	{
		local_scheduler_t ls;

		GO
		{
			for (int i = N - 1; i >= 0; --i)
			{
				co_await(c << i);
			}
		};

		this_scheduler()->run_until_notask();
	});

	go[&]() -> future_t<>
	{
		auto tstart = high_resolution_clock::now();

		int i;
		do
		{
			i = co_await c;
		} while (i > 0);

		auto dt = duration_cast<duration<double>>(high_resolution_clock::now() - tstart).count();
		std::cout << "channel buff=" << c.capacity() << ", w/r " << N << " times, cost time " << dt << "s" << std::endl;
	};

	this_scheduler()->run_until_notask();

	wr_th.join();
}

void resumable_main_channel()
{
	test_channel_read_first();
	std::cout << std::endl;

	test_channel_write_first();
	std::cout << std::endl;

	test_channel_performance_single_thread(1);
	test_channel_performance_single_thread(10);
	test_channel_performance_single_thread(100);
	test_channel_performance_single_thread(1000);

	test_channel_performance_double_thread(1);
	test_channel_performance_double_thread(10);
	test_channel_performance_double_thread(100);
	test_channel_performance_double_thread(1000);
}
