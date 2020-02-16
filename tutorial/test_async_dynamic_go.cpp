
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>

#include "librf.h"


static const int M = 10;

size_t dynamic_go_count = 0;
std::array<std::array<std::array<int32_t, M>, M>, 3> dynamic_cells;

void test_dynamic_go()
{
	auto co_star = [](int j) -> resumef::future_t<int>
	{
		for (int i = 0; i < M; ++i)
		{
			go[=]() -> resumef::generator_t<int>
			{
				for (int k = 0; k < M; ++k)
				{
					++dynamic_cells[j][i][k];
					++dynamic_go_count;

					std::cout << j << "  " << i << "  " << k << std::endl;
					co_yield k;
				}

				co_return M;
			};

			co_yield i;
		}

		co_return M;
	};
	go co_star(0);
	go co_star(1);
	go co_star(2);

	resumef::this_scheduler()->run_until_notask();

	std::cout << "dynamic_go_count = " << dynamic_go_count << std::endl;
	for (auto & j : dynamic_cells)
	{
		for (auto & i : j)
		{
			for (auto k : i)
				std::cout << k;
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}

void resumable_main_dynamic_go()
{
	test_dynamic_go();
}
