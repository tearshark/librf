
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

const size_t N = 1000000;

volatile size_t globalValue = 0;

void resumable_main_benchmark_mem()
{
	using namespace std::chrono;
	
	for (size_t i = 0; i < N; ++i)
	{
		go[=]()->resumef::generator_t<size_t>
		{
			for (size_t k = 0; k < 10; ++k)
			{
				globalValue += i * k;
				co_yield k;
			}
			co_return 0;
		};
	}

	resumef::this_scheduler()->run_until_notask();
	(void)_getch();
}
