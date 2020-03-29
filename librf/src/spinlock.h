//用于内部实现的循环锁

#pragma once

RESUMEF_NS
{
#if defined(RESUMEF_USE_CUSTOM_SPINLOCK)
	using spinlock = RESUMEF_USE_CUSTOM_SPINLOCK;
#else

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

		bool try_lock() noexcept
		{
			int val = FREE_VALUE;
			bool ret = lck.compare_exchange_strong(val, LOCKED_VALUE, std::memory_order_acq_rel);

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

	template<class _Ty, class _Cont = std::vector<_Ty>, class _Assemble = detail::_LockVectorAssembleT<_Ty, _Cont>>
	class scoped_lock_range { // class with destructor that unlocks mutexes
	public:
		explicit scoped_lock_range(_Cont& locks_)
			: _LkN(&locks_)
			, _LA(*_LkN)
		{
			detail::scoped_lock_range_lock_impl::_Lock_range(_LA);
		}
		explicit scoped_lock_range(_Cont& locks_, _Assemble& la_)
			: _LkN(&locks_)
			, _LA(la_)
		{
			detail::scoped_lock_range_lock_impl::_Lock_range(_LA);
		}

		explicit scoped_lock_range(std::adopt_lock_t, _Cont& locks_)
			: _LkN(&locks_)
			, _LA(*_LkN)
		{ // construct but don't lock
		}
		explicit scoped_lock_range(std::adopt_lock_t, _Cont& locks_, _Assemble& la_)
			: _LkN(&locks_)
			, _LA(la_)
		{ // construct but don't lock
		}

		~scoped_lock_range() noexcept
		{
			if (_LkN != nullptr)
				detail::scoped_lock_range_lock_impl::_Unlock_locks(0, (int)_LA.size(), _LA);
		}

		void unlock()
		{
			if (_LkN != nullptr)
			{
				_LkN = nullptr;
				detail::scoped_lock_range_lock_impl::_Unlock_locks(0, (int)_LA.size(), _LA);
			}
		}

		scoped_lock_range(const scoped_lock_range&) = delete;
		scoped_lock_range& operator=(const scoped_lock_range&) = delete;

		scoped_lock_range(scoped_lock_range&& _Right)
			: _LkN(_Right._LkN)
			, _LA(std::move(_Right._LA))
		{
			_Right._LkN = nullptr;
		}
		scoped_lock_range& operator=(scoped_lock_range&& _Right)
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
