
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>
#include <inttypes.h>

#include "librf.h"

using namespace resumef;

void test_when_any()
{
	using namespace std::chrono;

	GO
	{
		auto vals = co_await when_any();

		vals = co_await when_any(
			[]() ->future_t<int>
			{
				auto dt = rand() % 1000;
				co_await sleep_for(1ms * dt);
				std::cout << dt << "@a" << std::endl;

				return dt;
			}(),
			[]() ->future_vt
			{
				auto dt = rand() % 1000;
				co_await sleep_for(1ms * dt);
				std::cout << dt << "@b" << std::endl;
			}(),
			[]() ->future_vt
			{
				auto dt = rand() % 1000;
				co_await sleep_for(1ms * dt);
				std::cout << dt << "@c" << std::endl;
			}());

		if (vals.first == 0)
			std::cout << "first done! value is " << std::any_cast<int>(vals.second) << std::endl;
		else
			std::cout << "any done! index is " << vals.first << std::endl;

		co_await sleep_for(1010ms);
		std::cout << std::endl;

		auto my_sleep = [](const char * name) -> future_t<int>
		{
			auto dt = rand() % 1000;
			co_await sleep_for(1ms * dt);
			std::cout << dt << "@" << name << std::endl;

			return dt;
		};

		std::vector<future_t<int> > v{ my_sleep("g"), my_sleep("h"), my_sleep("i") };
		vals = co_await when_any(std::begin(v), std::end(v));
		std::cout << "any range done! index is " << vals.first << ", valus is " << std::any_cast<int>(vals.second) << std::endl;
	};
	this_scheduler()->run_until_notask();
}

void test_when_all()
{
	using namespace std::chrono;

	auto my_sleep = [](const char * name) -> future_t<int>
	{
		auto dt = rand() % 1000;
		co_await sleep_for(1ms * dt);
		std::cout << dt << "@" << name << std::endl;

		return dt;
	};

	auto my_sleep_v = [](const char * name) -> future_vt
	{
		auto dt = rand() % 1000;
		co_await sleep_for(1ms * dt);
		std::cout << dt << "@" << name << std::endl;
	};


	GO
	{
		co_await when_all();
		std::cout << "when all: zero!" << std::endl << std::endl;

		auto [a, b] = co_await when_all(my_sleep("a"), my_sleep_v("b"));
		(void)b;		//b is std::ignore
		std::cout << "when all:" << a << std::endl << std::endl;

		auto c = co_await my_sleep("c");
		std::cout << "when all:" << c << std::endl << std::endl;

		auto [d, e, f] = co_await when_all(my_sleep("d"), my_sleep_v("e"), my_sleep("f"));
		(void)e;		//e is std::ignore
		std::cout << "when all:" << d << "," << f << std::endl << std::endl;

		std::vector<future_t<int> > v{ my_sleep("g"), my_sleep("h"), my_sleep("i") };
		auto vals = co_await when_all(std::begin(v), std::end(v));
		std::cout << "when all:" << vals[0] << "," << vals[1] << "," << vals[2] << "," << std::endl << std::endl;

		std::cout << "all range done!" << std::endl;
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
