
//用于内部实现的循环锁

#pragma once

#include "def.h"

namespace resumef
{
	struct spinlock
	{
		volatile std::atomic_flag lck;

		spinlock()
		{
			lck.clear();
		}

		void lock()
		{
			if (std::atomic_flag_test_and_set_explicit(&lck, std::memory_order_acquire))
			{
				using namespace std::chrono;
				auto dt = 1ms;
				while (std::atomic_flag_test_and_set_explicit(&lck, std::memory_order_acquire))
				{
					std::this_thread::sleep_for(dt);
					dt *= 2;
				}
			}
		}

		bool try_lock()
		{
			bool ret = !std::atomic_flag_test_and_set_explicit(&lck, std::memory_order_acquire);
			return ret;
		}

		void unlock()
		{
			std::atomic_flag_clear_explicit(&lck, std::memory_order_release);
		}
	};
}
