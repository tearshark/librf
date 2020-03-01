//用于内部实现的循环锁

#pragma once

RESUMEF_NS
{
	struct spinlock
	{
		static const size_t MAX_ACTIVE_SPIN = 4000;
		static const int FREE_VALUE = 0;
		static const int LOCKED_VALUE = 1;

		volatile std::atomic<int> lck;
#if _DEBUG
		std::thread::id owner_thread_id;
#endif

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
#if _DEBUG
				assert(owner_thread_id != std::this_thread::get_id());
#endif

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

#if _DEBUG
			owner_thread_id = std::this_thread::get_id();
#endif
		}

		bool try_lock()
		{
			int val = FREE_VALUE;
			bool ret = lck.compare_exchange_weak(val, LOCKED_VALUE, std::memory_order_acquire);

#if _DEBUG
			if (ret) owner_thread_id = std::this_thread::get_id();
#endif

			return ret;
		}

		void unlock()
		{
#if _DEBUG
			owner_thread_id = std::thread::id();
#endif
			lck.store(FREE_VALUE, std::memory_order_release);
		}
	};
}
