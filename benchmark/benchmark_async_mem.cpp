
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "librf.h"

const size_t N = 2000000;
const size_t LOOP_COUNT = 50;

volatile size_t globalValue = 0;

void resumable_main_benchmark_mem(bool wait_key)
{
	using namespace std::chrono;
	
	for (size_t i = 0; i < N; ++i)
	{
		go[=]()->resumef::generator_t<size_t>
		{
			for (size_t k = 0; k < LOOP_COUNT; ++k)
			{
				globalValue += i * k;
				co_yield k;
			}
			co_return 0;
		};
	}

	resumef::this_scheduler()->run_until_notask();
	if (wait_key)
	{
		std::cout << "press any key to continue." << std::endl;
		(void)getchar();
	}
}

//clang : 平均 210字节
//msvc : 平均600字节
