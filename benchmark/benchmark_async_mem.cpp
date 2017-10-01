
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>

#include "librf.h"

const size_t N = 1000000;

void resumable_main_benchmark_mem()
{
	using namespace std::chrono;

	for (size_t i = 0; i < N; ++i)
	{
		GO
		{
			for(size_t k = 0; k<100; ++k)
				co_await resumef::sleep_for(10s);
		};
	}

	resumef::this_scheduler()->run_until_notask();
}
