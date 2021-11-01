
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "librf/librf.h"

using namespace librf;

void resumable_main_timer()
{
	using namespace std::chrono;

	auto th = this_scheduler()->timer()->add_handler(system_clock::now() + 5s, 
		[](bool bValue) 
		{
			if (bValue)
				std::cout << "timer canceled." << std::endl;
			else
				std::cout << "timer after 5s." << std::endl;
		});

	auto th2 = this_scheduler()->timer()->add_handler(1s, 
		[&th](bool)
		{
			std::cout << "timer after 1s." << std::endl;
			th.stop();
		});

	this_scheduler()->run_until_notask();


	th2.stop();	//but th2 is invalid
}

int main()
{
	resumable_main_timer();
	return 0;
}
