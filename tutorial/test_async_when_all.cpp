
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>
#include <inttypes.h>

#include "librf.h"

#if _HAS_CXX17 || RESUMEF_USE_BOOST_ANY

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
			[]() ->future_t<>
			{
				auto dt = rand() % 1000;
				co_await sleep_for(1ms * dt);
				std::cout << dt << "@b" << std::endl;
			}(),
			[]() ->future_t<>
			{
				auto dt = rand() % 1000;
				co_await sleep_for(1ms * dt);
				std::cout << dt << "@c" << std::endl;
			}());

		if (vals.first == 0)
			std::cout << "first done! value is " << resumef::any_cast<int>(vals.second) << std::endl;
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
		std::cout << "any range done! index is " << vals.first << ", valus is " << resumef::any_cast<int>(vals.second) << std::endl;
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

	auto my_sleep_v = [](const char * name) -> future_t<>
	{
		auto dt = rand() % 1000;
		co_await sleep_for(1ms * dt);
		std::cout << dt << "@" << name << std::endl;
	};


	GO
	{
		co_await when_all();
		std::cout << "when all: zero!" << std::endl << std::endl;

		auto ab = co_await when_all(my_sleep("a"), my_sleep_v("b"));
		//ab.1 is std::ignore
		std::cout << "when all:" << std::get<0>(ab) << std::endl << std::endl;

		auto c = co_await my_sleep("c");
		std::cout << "when all:" << c << std::endl << std::endl;

		auto def = co_await when_all(my_sleep("d"), my_sleep_v("e"), my_sleep("f"));
		//def.1 is std::ignore
		std::cout << "when all:" << std::get<0>(def) << "," << std::get<2>(def) << std::endl << std::endl;

		std::vector<future_t<int> > v{ my_sleep("g"), my_sleep("h"), my_sleep("i") };
		auto vals = co_await when_all(std::begin(v), std::end(v));
		std::cout << "when all:" << vals[0] << "," << vals[1] << "," << vals[2] << "," << std::endl << std::endl;

		std::cout << "all range done!" << std::endl;
	};
	this_scheduler()->run_until_notask();
}
#endif

void resumable_main_when_all()
{
#if _HAS_CXX17 || RESUMEF_USE_BOOST_ANY
	srand((uint32_t)time(nullptr));

	test_when_any();
	std::cout << std::endl;
	test_when_all();
#endif
}

