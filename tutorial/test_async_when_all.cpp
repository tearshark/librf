
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>
#include <experimental/resumable>

#include "librf.h"

using namespace resumef;

void test_when_any()
{
	using namespace std::chrono;

	GO
	{
		co_await when_any(
			[]() ->future_vt
			{
				auto dt = rand() % 1000;
				co_await sleep_for(1ms * dt);
				std::cout << dt << "@1000" << std::endl;
			}(),
			[]() ->future_vt
			{
				auto dt = rand() % 2000;
				co_await sleep_for(1ms * dt);
				std::cout << dt << "@2000" << std::endl;
			}(),
			[]() ->future_vt
			{
				auto dt = rand() % 3000;
				co_await sleep_for(1ms * dt);
				std::cout << dt << "@3000" << std::endl;
			}());
		std::cout << "any done!" << std::endl;
	};
	this_scheduler()->run_until_notask();
}

void test_when_all()
{
	using namespace std::chrono;

	auto my_sleep = [](const char * name) -> future_vt
	{
		auto dt = rand() % 1000;
		co_await sleep_for(1ms * dt);
		std::cout << dt << "@" << name << std::endl;
	};

	GO
	{
		co_await when_all();
		std::cout << "zero!" << std::endl;

		co_await when_all(my_sleep("a"), my_sleep("b"));
		co_await my_sleep("c");
		co_await when_all(my_sleep("d"), my_sleep("e"), my_sleep("f"));
		std::cout << "all done!" << std::endl;
	};
	this_scheduler()->run_until_notask();
}

void resumable_main_when_all()
{
	srand((uint32_t)time(nullptr));

	test_when_any();
	std::cout << std::endl;
	test_when_all();
}
