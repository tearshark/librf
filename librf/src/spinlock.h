
//用于内部实现的循环锁

#pragma once

#include "def.h"

RESUMEF_NS
{
	struct spinlock
	{
		static const size_t MAX_ACTIVE_SPIN = 4000;
		static const int FREE_VALUE = 0;
		static const int LOCKED_VALUE = 1;

		volatile std::atomic<int> lck;

		spinlock()
		{
			lck = FREE_VALUE;
		}

		void lock()
		{
			using namespace std::chrono;

			int val = FREE_VALUE;
			if (!lck.compare_exchange_weak(val, LOCKED_VALUE, std::memory_order_acquire))
			{
				size_t spinCount = 0;
				auto dt = 1ms;
				do
				{
					while (lck.load(std::memory_order_relaxed) != FREE_VALUE)
					{
						if (spinCount < MAX_ACTIVE_SPIN)
							++spinCount;
						else
						{
							std::this_thread::sleep_for(dt);
							dt *= 2;
						}
					}

					val = FREE_VALUE;
				} while (!lck.compare_exchange_weak(val, LOCKED_VALUE, std::memory_order_acquire));
			}
		}

		bool try_lock()
		{
			int val = FREE_VALUE;
			bool ret = lck.compare_exchange_weak(val, LOCKED_VALUE, std::memory_order_acquire);
			return ret;
		}

		void unlock()
		{
			lck.store(FREE_VALUE, std::memory_order_release);
		}
	};
}
