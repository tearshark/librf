//用于内部实现的循环锁

#pragma once

namespace resumef
{
#if defined(RESUMEF_USE_CUSTOM_SPINLOCK)
	using spinlock = RESUMEF_USE_CUSTOM_SPINLOCK;
#else

	/**
	 * @brief 一个自旋锁实现。
	 */
	struct spinlock
	{
		static const size_t MAX_ACTIVE_SPIN = 4000;
		static const size_t MAX_YIELD_SPIN = 8000;
		static const int FREE_VALUE = 0;
		static const int LOCKED_VALUE = 1;

		std::atomic<int> lck;
#if _DEBUG
		std::thread::id owner_thread_id;
#endif

		/**
		 * @brief 初始为未加锁。
		 */
		spinlock() noexcept
		{
			lck = FREE_VALUE;
		}

		/**
		 * @brief 获得锁。会一直阻塞线程直到获得锁。
		 */
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
							dt = (std::min)(std::chrono::milliseconds{ 128 }, dt * 2);
						}
					}

					val = FREE_VALUE;
				} while (!lck.compare_exchange_weak(val, LOCKED_VALUE, std::memory_order_acq_rel));
			}

#if _DEBUG
			owner_thread_id = std::this_thread::get_id();
#endif
		}

		/**
		 * @brief 尝试获得锁一次。
		 */
		bool try_lock() noexcept
		{
			int val = FREE_VALUE;
			bool ret = lck.compare_exchange_strong(val, LOCKED_VALUE, std::memory_order_acq_rel);

#if _DEBUG
			if (ret) owner_thread_id = std::this_thread::get_id();
#endif

			return ret;
		}

		/**
		 * @brief 释放锁。
		 */
		void unlock() noexcept
		{
#if _DEBUG
			owner_thread_id = std::thread::id();
#endif
			lck.store(FREE_VALUE, std::memory_order_release);
		}
	};

#endif

#ifndef DOXYGEN_SKIP_PROPERTY
	namespace detail
	{
		template<class _Ty, class _Cont = std::vector<_Ty>>
		struct _LockVectorAssembleT
		{
		private:
			_Cont& _Lks;
		public:
			_LockVectorAssembleT(_Cont& _LkN)
				: _Lks(_LkN)
			{}
			size_t size() const
			{
				return _Lks.size();
			}
			_Ty& operator[](int _Idx)
			{
				return _Lks[_Idx];
			}
			void _Lock_ref(_Ty& _LkN) const
			{
				_LkN.lock();
			}
			bool _Try_lock_ref(_Ty& _LkN) const
			{
				return _LkN.try_lock();
			}
			void _Unlock_ref(_Ty& _LkN) const
			{
				_LkN.unlock();
			}
			void _Yield() const
			{
				std::this_thread::yield();
			}
			void _ReturnValue() const;
			template<class U>
			U _ReturnValue(U v) const;
		};

		template<class _Ty, class _Cont>
		struct _LockVectorAssembleT<std::reference_wrapper<_Ty>, _Cont>
		{
		private:
			_Cont& _Lks;
		public:
			_LockVectorAssembleT(_Cont& _LkN)
				: _Lks(_LkN)
			{}
			size_t size() const
			{
				return _Lks.size();
			}
			std::reference_wrapper<_Ty> operator[](int _Idx)
			{
				return _Lks[_Idx];
			}
			void _Lock_ref(std::reference_wrapper<_Ty> _LkN) const
			{
				_LkN.get().lock();
			}
			void _Unlock_ref(std::reference_wrapper<_Ty> _LkN) const
			{
				_LkN.get().unlock();
			}
			bool _Try_lock_ref(std::reference_wrapper<_Ty> _LkN) const
			{
				return _LkN.get().try_lock();
			}
			void _Yield() const
			{
				std::this_thread::yield();
			}
			void _ReturnValue() const;
			template<class U>
			U _ReturnValue(U v) const;
		};

#define LOCK_ASSEMBLE_NAME(fnName) scoped_lock_range_##fnName
#define LOCK_ASSEMBLE_AWAIT(a) (a)
#define LOCK_ASSEMBLE_RETURN(a) return (a)
#include "without_deadlock_assemble.inl"
#undef LOCK_ASSEMBLE_NAME
#undef LOCK_ASSEMBLE_AWAIT
#undef LOCK_ASSEMBLE_RETURN
	}
#endif	//DOXYGEN_SKIP_PROPERTY

	/**
	 * @brief 无死锁的批量枷锁。
	 * @param _Ty 锁的类型。例如std::mutex，resumef::spinlock，resumef::mutex_t(线程用法）均可。
	 * @param _Cont 容纳一批锁的容器。
	 * @param _Assemble 与_Cont配套的锁集合，特化了如何操作_Ty。
	 */
	template<class _Ty, class _Cont = std::vector<_Ty>, class _Assemble = detail::_LockVectorAssembleT<_Ty, _Cont>>
	class batch_lock_t
	{
	public:
		/**
		 * @brief 通过锁容器构造，并立刻应用加锁算法。
		 */
		explicit batch_lock_t(_Cont& locks_)
			: _LkN(&locks_)
			, _LA(*_LkN)
		{
			detail::scoped_lock_range_lock_impl::_Lock_range(_LA);
		}
		
		/**
		 * @brief 通过锁容器和锁集合构造，并立刻应用加锁算法。
		 */
		explicit batch_lock_t(_Cont& locks_, _Assemble& la_)
			: _LkN(&locks_)
			, _LA(la_)
		{
			detail::scoped_lock_range_lock_impl::_Lock_range(_LA);
		}

		/**
		 * @brief 通过锁容器构造，容器里的锁已经全部获得。
		 */
		explicit batch_lock_t(std::adopt_lock_t, _Cont& locks_)
			: _LkN(&locks_)
			, _LA(*_LkN)
		{ // construct but don't lock
		}

		/**
		 * @brief 通过锁容器和锁集合构造，容器里的锁已经全部获得。
		 */
		explicit batch_lock_t(std::adopt_lock_t, _Cont& locks_, _Assemble& la_)
			: _LkN(&locks_)
			, _LA(la_)
		{ // construct but don't lock
		}

		/**
		 * @brief 析构函数里，释放容器里的锁。
		 */
		~batch_lock_t() noexcept
		{
			if (_LkN != nullptr)
				detail::scoped_lock_range_lock_impl::_Unlock_locks(0, (int)_LA.size(), _LA);
		}

		/**
		 * @brief 手工释放容器里的锁，析构函数里将不再有释放操作。
		 */
		void unlock()
		{
			if (_LkN != nullptr)
			{
				_LkN = nullptr;
				detail::scoped_lock_range_lock_impl::_Unlock_locks(0, (int)_LA.size(), _LA);
			}
		}

		/**
		 * @brief 不支持拷贝构造。
		 */
		batch_lock_t(const batch_lock_t&) = delete;

		/**
		 * @brief 不支持拷贝赋值。
		 */
		batch_lock_t& operator=(const batch_lock_t&) = delete;

		/**
		 * @brief 支持移动构造。
		 */
		batch_lock_t(batch_lock_t&& _Right)
			: _LkN(_Right._LkN)
			, _LA(std::move(_Right._LA))
		{
			_Right._LkN = nullptr;
		}

		/**
		 * @brief 支持移动赋值。
		 */
		batch_lock_t& operator=(batch_lock_t&& _Right)
		{
			if (this != &_Right)
			{
				_LkN = _Right._LkN;
				_Right._LkN = nullptr;

				_LA = std::move(_Right._LA);
			}
		}
	private:
		_Cont* _LkN;
		_Assemble _LA;
	};
}
