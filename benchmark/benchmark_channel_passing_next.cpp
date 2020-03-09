
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <deque>
#include <mutex>

#include "librf.h"

using namespace resumef;
using namespace std::chrono;
using namespace std::literals;

const static auto MaxNum = 100000;
const static auto LoopCount = 100;

using int_channel_ptr = std::shared_ptr<channel_t<intptr_t>>;

static future_t<> passing_next(channel_t<intptr_t> rd, channel_t<intptr_t> wr)
{
	for (int i = 0; i < LoopCount; ++i)
	{
		intptr_t value = co_await rd;
		co_await wr.write(value + 1);
	}
}

void benchmark_main_channel_passing_next()
{
	int_channel_ptr head = std::make_shared<channel_t<intptr_t>>(1);
	int_channel_ptr in = head;
	int_channel_ptr tail = nullptr;

	for (int i = 0; i < MaxNum; ++i)
	{
		tail = std::make_shared<channel_t<intptr_t>>(1);
		go passing_next(*in, *tail);
		in = tail;
	}

	GO
	{
		for (int i = 0; i < LoopCount; ++i)
		{
			auto tstart = high_resolution_clock::now();

			co_await (*head << 0);
			intptr_t value = co_await *tail;

			auto dt = duration_cast<duration<double>>(high_resolution_clock::now() - tstart).count();
			std::cout << value << " cost time " << dt << "s" << std::endl;
		}
	};

	this_scheduler()->run_until_notask();
}
