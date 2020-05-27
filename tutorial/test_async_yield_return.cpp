
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "librf.h"

using namespace resumef;

generator_t<int> test_yield_int()
{
	std::cout << "1 will yield return" << std::endl;
	co_yield 1;
	std::cout << "2 will yield return" << std::endl;
	co_yield 2;
	std::cout << "3 will yield return" << std::endl;
	co_yield 3;
	std::cout << "4 will return" << std::endl;
	co_return 4;

	std::cout << "5 will never yield return" << std::endl;
	co_yield 5;
}

/*不能编译*/
/*
auto test_yield_void()
{
	std::cout << "1 will yield return" << std::endl;
	co_yield ;
	std::cout << "2 will yield return" << std::endl;
	co_yield ;
	std::cout << "3 will yield return" << std::endl;
	co_yield ;
	std::cout << "4 will return" << std::endl;
	co_return ;

	std::cout << "5 will never yield return" << std::endl;
	co_yield ;
}
*/

auto test_yield_void() -> generator_t<>
{
	std::cout << "block 1 will yield return" << std::endl;
	co_yield_void;
	std::cout << "block 2 will yield return" << std::endl;
	co_yield_void;
	std::cout << "block 3 will yield return" << std::endl;
	co_yield_void;
	std::cout << "block 4 will return" << std::endl;
	co_return_void;

	std::cout << "block 5 will never yield return" << std::endl;
	co_yield_void;
}

auto test_yield_future() -> future_t<int64_t>
{
	std::cout << "future 1 will yield return" << std::endl;
	co_yield 1;
	std::cout << "future 2 will yield return" << std::endl;
	co_yield 2;
	std::cout << "future 3 will yield return" << std::endl;
	co_yield 3;
	std::cout << "future 4 will return" << std::endl;
	co_return 4;

	std::cout << "future 5 will never yield return" << std::endl;
	co_yield 5;
}

void resumable_main_yield_return()
{
	std::cout << __FUNCTION__ << std::endl;
	for (int i : test_yield_int())
	{
		std::cout << i << " had return" << std::endl;
	}

	go test_yield_int();
	this_scheduler()->run_until_notask();

	go test_yield_void();
	this_scheduler()->run_until_notask();

	go test_yield_future();
	this_scheduler()->run_until_notask();
}
