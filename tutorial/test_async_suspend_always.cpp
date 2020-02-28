
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

using namespace resumef;

future_t<> test_loop_sleep(size_t _N, const char * ch)
{
	using namespace std::chrono;

	for (size_t i = 0; i < _N; ++i)
	{
		co_await resumef::sleep_for(100ms);
		std::cout << ch;
	}
	std::cout << std::endl;
}

future_t<> test_recursive_await()
{
	std::cout << "A:---1" << std::endl;
	co_await test_loop_sleep(5, "=");

	std::cout << "A:---2" << std::endl;
	co_await test_loop_sleep(6, "=");

	std::cout << "A:---3" << std::endl;
	co_await test_loop_sleep(7, "=");

	std::cout << "A:---4" << std::endl;
}

future_t<> test_recursive_go()
{
	std::cout << "B:---1" << std::endl;
	co_await test_loop_sleep(3, "+");

	std::cout << "B:---2" << std::endl;
	go test_loop_sleep(8, "*");

	std::cout << "B:---3" << std::endl;
	co_await test_loop_sleep(4, "+");

	std::cout << "B:---4" << std::endl;
}

void resumable_main_suspend_always()
{
	go test_recursive_await();
	go test_recursive_go();
	this_scheduler()->run_until_notask();
}

/*
resume from 0000016B8477CE00 on thread 7752

resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.
resume from 0000016B8477CE00 on thread 7752

resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.
resume from 0000016B8477CE00 on thread 7752

resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.resume from 0000016B847726C0 on thread 7752
.
resume from 0000016B8477CE00 on thread 7752

说明有四个协程对象（其中三个对象的内存被复用，表现为地址一样）
test_recursive_await	->		0000016B8477CE00
test_loop_sleep<5>		->		0000016B847726C0
test_loop_sleep<6>		->		0000016B847726C0
test_loop_sleep<7>		->		0000016B847726C0
*/