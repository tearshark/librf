
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "librf/librf.h"

const size_t N = 5000000;
const size_t LOOP_COUNT = 50;

std::atomic<size_t> globalValue{0};

void resumable_main_benchmark_mem(bool wait_key)
{
	using namespace std::chrono;
	
	for (size_t i = 0; i < N; ++i)
	{
		go[=]()->librf::generator_t<size_t>
		{
			for (size_t k = 0; k < LOOP_COUNT; ++k)
			{
				globalValue += i * k;
				co_yield k;
			}
			co_return 0;
		};
	}

	librf::this_scheduler()->run_until_notask();
	if (wait_key)
	{
		std::cout << "press any key to continue." << std::endl;
		(void)getchar();
	}
}

//clang : 平均 256 字节
//msvc : 平均 304 字节(vs2022,17.7.4)

#if LIBRF_TUTORIAL_STAND_ALONE
int main()
{
	resumable_main_benchmark_mem(false);
	return 0;
}
#endif
