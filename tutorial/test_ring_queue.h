#pragma once

#include <cstdint>
#include <thread>
#include <cstdlib>
#include <iostream>
#include <random>

template<class ring_queue>
bool test_ring_queue_thread(uint32_t seed, bool printResult)
{
	ring_queue q{ 12 };

	const size_t N = 1000000;
	std::atomic<int64_t> total_push = 0, total_pop = 0;

	std::thread th_push[2];
	for (auto& th : th_push)
	{
		th = std::thread([=, &q, &total_push]
			{
				std::mt19937 rd_generator{ seed };
				std::uniform_int_distribution<uint32_t> distribution{ 0, 9 };

				for (size_t i = 0; i < N; ++i)
				{
					int value = (int)distribution(rd_generator);
					total_push += value;

					while (!q.try_push(value))
						std::this_thread::yield();
				}

				if (printResult)
					std::cout << "+";
			});
	}

	std::thread th_pop[2];
	for (auto& th : th_pop)
	{
		th = std::thread([=, &q, &total_pop]
			{
				for (size_t i = 0; i < N; ++i)
				{
					int value;
					while (!q.try_pop(value))
						std::this_thread::yield();

					total_pop += value;
				}

				if (printResult)
					std::cout << "-";
			});
	}

	for (auto& th : th_push)
		th.join();
	for (auto& th : th_pop)
		th.join();

	//assert(total_push.load() == total_pop.load());

	if (printResult)
	{
		std::cout << std::endl;
		std::cout << "push = " << total_push.load() << ", pop = " << total_pop.load() << std::endl;
	}

	return total_push.load() == total_pop.load();
}

template<class ring_queue>
void test_ring_queue_simple()
{
	ring_queue rq_zero{ 0 };

	assert(rq_zero.empty());
	assert(rq_zero.full());
	assert(rq_zero.size() == 0);

	ring_queue rq{ 2 };

	assert(rq.size() == 0);
	assert(rq.empty());
	assert(!rq.full());

	assert(rq.try_push(10));
	assert(rq.size() == 1);
	assert(!rq.empty());
	assert(!rq.full());

	assert(rq.try_push(11));
	assert(rq.size() == 2);
	assert(!rq.empty());
	assert(rq.full());

	assert(!rq.try_push(12));

	int value;
	(void)value;
	assert(rq.try_pop(value) && value == 10);
	assert(rq.size() == 1);
	assert(!rq.empty());
	assert(!rq.full());

	assert(rq.try_pop(value) && value == 11);
	assert(rq.size() == 0);
	assert(rq.empty());
	assert(!rq.full());

	assert(!rq.try_pop(value));

	assert(rq.try_push(13));
	assert(rq.size() == 1);
	assert(!rq.empty());
	assert(!rq.full());

	assert(rq.try_pop(value) && value == 13);
	assert(rq.size() == 0);
	assert(rq.empty());
	assert(!rq.full());
}

template<class ring_queue>
void test_ring_queue()
{
	using namespace std::chrono;

	std::random_device rd{};

	test_ring_queue_simple<ring_queue>();

	auto tp_start = system_clock::now();

	for (int i = 0; i < 20; ++i)
	{
		if (test_ring_queue_thread<ring_queue>(rd(), false))
			std::cout << ".";
		else
			std::cout << "E";
	}

	auto dt = system_clock::now() - tp_start;
	std::cout << std::endl;
	std::cout << "cost time: " << duration_cast<duration<double>>(dt).count() << "s." << std::endl;
}
