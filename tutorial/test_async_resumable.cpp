
#include <chrono>
#include <iostream>
#include <string>

#include "librf.h"


static std::mutex lock_console;

template <typename T>
void dump(size_t idx, std::string name, T start, T end, intptr_t count)
{
	lock_console.lock();

	auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
	std::cout << idx << ":" << name << "    ";
	std::cout << count << "      " << ns << " ns      ";
	std::cout << (ns / count) << " ns/op" << "    ";
	std::cout << (count * 100ll * 1000ll / ns) << "w/cps" << std::endl;

	lock_console.unlock();
}

static const intptr_t N = 3000000;
//static const int N = 10;

auto yield_switch(intptr_t coro) -> resumef::generator_t<intptr_t>
{
	for (intptr_t i = N / coro; i > 0; --i)
		co_yield i;
	co_return 0;
}

void resumable_switch(intptr_t coro, size_t idx)
{
	resumef::local_scheduler_t ls;

	auto start = std::chrono::steady_clock::now();
	
	for (intptr_t i = 0; i < coro; ++i)
	{
		go yield_switch(coro);
	}
	auto middle = std::chrono::steady_clock::now();
	dump(idx, "BenchmarkCreate_" + std::to_string(coro), start, middle, coro);

	resumef::this_scheduler()->run_until_notask();

	auto end = std::chrono::steady_clock::now();
	dump(idx, "BenchmarkSwitch_" + std::to_string(coro), middle, end, N);
}

void resumable_main_resumable()
{
	resumable_switch(1, 99);

	resumable_switch(1, 0);
	resumable_switch(10, 0);
	resumable_switch(100, 0);
	resumable_switch(1000, 0);
	resumable_switch(10000, 0);
	resumable_switch(30000, 0);

/*
	std::thread works[32];
	for (size_t w = 1; w <= std::size(works); ++w)
	{
		for (size_t idx = 0; idx < w; ++idx)
			works[idx] = std::thread(&resumable_switch, 1000, idx);
		for (size_t idx = 0; idx < w; ++idx)
			works[idx].join();

		std::cout << std::endl << std::endl;
	}
*/
}
