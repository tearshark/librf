
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <inttypes.h>

#include "librf.h"

using namespace resumef;

#if !defined(__GNUC__)
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

				co_return dt;
			},
			[]() ->future_t<>
			{
				auto dt = rand() % 1000;
				co_await sleep_for(1ms * dt);
				std::cout << dt << "@b" << std::endl;
			},
			[]() ->future_t<>
			{
				auto dt = rand() % 1000;
				co_await sleep_for(1ms * dt);
				std::cout << dt << "@c" << std::endl;
			});

		if (vals.first == 0)
			std::cout << "first done! value is " << resumef::any_cast<int>(vals.second) << std::endl;
		else
			std::cout << "any done! index is " << vals.first << std::endl;

		co_await 1010ms;
		std::cout << std::endl;

		auto my_sleep = [](const char * name) -> future_t<int>
		{
			auto dt = rand() % 1000;
			co_await sleep_for(1ms * dt);
			std::cout << dt << "@" << name << std::endl;

			co_return dt;
		};

		std::vector<future_t<int> > v{ my_sleep("g"), my_sleep("h"), my_sleep("i") };
		//vals = co_await when_any(*this_scheduler(), std::begin(v), std::end(v));
		//vals = co_await when_any(std::begin(v), std::end(v));
		vals = co_await when_any(v);

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

		co_return dt;
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

		auto vals1 = co_await when_all(
			[]() ->future_t<int>
			{
				auto dt = rand() % 1000;
				co_await sleep_for(1ms * dt);
				std::cout << dt << "@i" << std::endl;

				co_return dt;
			},
			[]() ->future_t<>
			{
				auto dt = rand() % 1000;
				co_await sleep_for(1ms * dt);
				std::cout << dt << "@j" << std::endl;
			},
			[]() ->future_t<>
			{
				auto dt = rand() % 1000;
				co_await sleep_for(1ms * dt);
				std::cout << dt << "@k" << std::endl;
			});

		std::cout << "when all - 1:" << std::get<0>(vals1) << std::endl << std::endl;

		auto ab = co_await when_all(my_sleep("a"), my_sleep_v("b"));
		//ab.1 is std::ignore
		std::cout << "when all - 2:" << std::get<0>(ab) << std::endl << std::endl;

		auto c = co_await my_sleep("c");
		std::cout << "when all - 3:" << c << std::endl << std::endl;

		auto def = co_await when_all(my_sleep("d"), my_sleep_v("e"), my_sleep("f"));
		//def.1 is std::ignore
		std::cout << "when all - 4:" << std::get<0>(def) << "," << std::get<2>(def) << std::endl << std::endl;

		std::vector<future_t<int> > v{ my_sleep("g"), my_sleep("h"), my_sleep("i") };
		//auto vals = co_await when_all(*this_scheduler(), std::begin(v), std::end(v));
		//auto vals = co_await when_all(*this_scheduler(), v);
		auto vals = co_await when_all(v);
		std::cout << "when all - 5:" << vals[0] << "," << vals[1] << "," << vals[2] << "," << std::endl << std::endl;

		std::cout << "all range done!" << std::endl;
	};
	this_scheduler()->run_until_notask();
}

void test_when_any_mix()
{
	using namespace std::chrono;

	event_t evt;

	GO
	{
		auto vals = co_await when_any(
			[]() ->future_t<int>
			{
				auto dt = rand() % 200;
				co_await sleep_for(1ms * dt);
				co_return dt;
			},
			evt.wait(),
			sleep_for(100ms)
			);

		if (vals.first == 0)
			std::cout << "first done! value is " << resumef::any_cast<int>(vals.second) << std::endl;
		else
			std::cout << "any done! index is " << vals.first << std::endl;
	};
	
	GO
	{
		auto dt = rand() % 120;
		co_await sleep_for(1ms * dt);
		evt.signal();
	};

	this_scheduler()->run_until_notask();
}

//这能模拟golang的select吗?
void test_when_select()
{
	using namespace std::chrono;

	channel_t<int> ch1, ch2;

	GO
	{
		auto vals = co_await when_any(
			sleep_for(60ms),
			[=]() ->future_t<int>
			{
				int val = co_await ch1;
				co_return val;
			},
			[=]() ->future_t<int>
			{
				int val = co_await ch2;
				co_return val;
			}
			);

		if (vals.first == 0)
			std::cout << "time out!" << std::endl;
		else
			std::cout << "index is " << vals.first << ", value is " << resumef::any_cast<int>(vals.second) << std::endl;
	};

	GO
	{
		auto dt = rand() % 120;
		co_await sleep_for(1ms * dt);
		co_await (ch1 << (int)dt);
	};

	GO
	{
		auto dt = rand() % 120;
		co_await sleep_for(1ms * dt);
		co_await (ch2 << (int)dt);
	};

	this_scheduler()->run_until_notask();
}
#endif

void resumable_main_when_all()
{
#if !defined(__GNUC__)
	srand((uint32_t)time(nullptr));

	test_when_any();
	std::cout << std::endl;
	
	test_when_all();
	std::cout << std::endl;

	for(int i = 0; i < 10; ++i)
		test_when_any_mix();

	for (int i = 0; i < 10; ++i)
		test_when_select();
#endif
}

