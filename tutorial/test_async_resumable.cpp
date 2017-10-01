
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>

#include "librf.h"


//static const int N = 100000000;
static const int N = 10;

template <typename T>
void dump(std::string name, int n, T start, T end)
{
	auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
	std::cout << name << "    " << n << "      " << ns << " ns      " << ns / n << " ns/op" << std::endl;
}

auto yield_switch(int coro)
{
	for (int i = 0; i < N / coro; ++i)
		co_yield i;
	return N / coro;
}

void resumable_switch(int coro)
{
	auto start = std::chrono::steady_clock::now();
	
	for (int i = 0; i < coro; ++i)
	{
		//go yield_switch(coro);
		go [=]
		{
			for (int i = 0; i < N / coro; ++i)
				co_yield i;
			return N / coro;
		};
	}
	resumef::this_scheduler()->run_until_notask();

	auto end = std::chrono::steady_clock::now();
	dump("BenchmarkSwitch_" + std::to_string(coro), N, start, end);
}

void resumable_main_resumable()
{
	resumable_switch(1);
	resumable_switch(1000);
	resumable_switch(1000000);
}
