//用于内部实现的循环锁

#pragma once

RESUMEF_NS
{
	struct spinlock
	{
		static const size_t MAX_ACTIVE_SPIN = 1000;
		static const size_t MAX_YIELD_SPIN = 4000;
		static const int FREE_VALUE = 0;
		static const int LOCKED_VALUE = 1;

		volatile std::atomic<int> lck;
#if _DEBUG
		std::thread::id owner_thread_id;
#endif

		spinlock() noexcept
		{
			lck = FREE_VALUE;
		}

		void lock() noexcept
		{
			int val = FREE_VALUE;
			if (!lck.compare_exchange_weak(val, LOCKED_VALUE, std::memory_order_acq_rel))
			{
#if _DEBUG
				//诊断错误的用法：进行了递归锁调用
				assert(owner_thread_id != std::this_thread::get_id());
#endif

				size_t spinCount = 0;
				auto dt = std::chrono::milliseconds{ 1 };
				do
				{
					while (lck.load(std::memory_order_relaxed) != FREE_VALUE)
					{
						if (spinCount < MAX_ACTIVE_SPIN)
						{
							++spinCount;
						}
						else if (spinCount < MAX_YIELD_SPIN)
						{
							++spinCount;
							std::this_thread::yield();
						}
						else
						{
							std::this_thread::sleep_for(dt);
							dt *= 2;
						}
					}

					val = FREE_VALUE;
				} while (!lck.compare_exchange_weak(val, LOCKED_VALUE, std::memory_order_acq_rel));
			}

#if _DEBUG
			owner_thread_id = std::this_thread::get_id();
#endif
		}

		bool try_lock() noexcept
		{
			int val = FREE_VALUE;
			bool ret = lck.compare_exchange_weak(val, LOCKED_VALUE, std::memory_order_acq_rel);

#if _DEBUG
			if (ret) owner_thread_id = std::this_thread::get_id();
#endif

			return ret;
		}

		void unlock() noexcept
		{
#if _DEBUG
			owner_thread_id = std::thread::id();
#endif
			lck.store(FREE_VALUE, std::memory_order_release);
		}
	};
}
