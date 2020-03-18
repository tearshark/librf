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
	
	namespace detail
	{
		template<class _Ty>
		void _Lock_ref(_Ty& _LkN)
		{
			_LkN.lock();
		}
		template<class _Ty>
		void _Lock_ref(std::reference_wrapper<_Ty> _LkN)
		{
			_LkN.get().lock();
		}
		template<class _Ty>
		void _Unlock_ref(_Ty& _LkN)
		{
			_LkN.unlock();
		}
		template<class _Ty>
		void _Unlock_ref(std::reference_wrapper<_Ty> _LkN)
		{
			_LkN.get().unlock();
		}
		template<class _Ty>
		bool _Try_lock_ref(_Ty& _LkN)
		{
			return _LkN.try_lock();
		}
		template<class _Ty>
		bool _Try_lock_ref(std::reference_wrapper<_Ty> _LkN)
		{
			return _LkN.get().try_lock();
		}

		template<class _Ty>
		void _Lock_from_locks(const int _Target, std::vector<_Ty>& _LkN) { // lock _LkN[_Target]
			_Lock_ref(_LkN[_Target]);
		}
		// FUNCTION TEMPLATE _Try_lock_from_locks
		template<class _Ty>
		bool _Try_lock_from_locks(const int _Target, std::vector<_Ty>& _LkN) { // try to lock _LkN[_Target]
			return _Try_lock_ref(_LkN[_Target]);
		}

		// FUNCTION TEMPLATE _Unlock_locks
		template<class _Ty>
		void _Unlock_locks(int _First, int _Last, std::vector<_Ty>& _LkN) noexcept /* terminates */ {
			for (; _First != _Last; ++_First) {
				_Unlock_ref(_LkN[_First]);
			}
		}

		// FUNCTION TEMPLATE try_lock
		template<class _Ty>
		int _Try_lock_range(const int _First, const int _Last, std::vector<_Ty>& _LkN) {
			int _Next = _First;
			try {
				for (; _Next != _Last; ++_Next) {
					if (!_Try_lock_from_locks(_Next, _LkN)) { // try_lock failed, backout
						_Unlock_locks(_First, _Next, _LkN);
						return _Next;
					}
				}
			}
			catch (...) {
				_Unlock_locks(_First, _Next, _LkN);
				throw;
			}

			return -1;
		}

		// FUNCTION TEMPLATE lock
		template<class _Ty>
		int _Lock_attempt(const int _Hard_lock, std::vector<_Ty>& _LkN) {
			// attempt to lock 3 or more locks, starting by locking _LkN[_Hard_lock] and trying to lock the rest
			_Lock_from_locks(_Hard_lock, _LkN);
			int _Failed = -1;
			int _Backout_start = _Hard_lock; // that is, unlock _Hard_lock

			try {
				_Failed = _Try_lock_range(0, _Hard_lock, _LkN);
				if (_Failed == -1) {
					_Backout_start = 0; // that is, unlock [0, _Hard_lock] if the next throws
					_Failed = _Try_lock_range(_Hard_lock + 1, (int)_LkN.size(), _LkN);
					if (_Failed == -1) { // we got all the locks
						return -1;
					}
				}
			}
			catch (...) {
				_Unlock_locks(_Backout_start, _Hard_lock + 1, _LkN);
				throw;
			}

			// we didn't get all the locks, backout
			_Unlock_locks(_Backout_start, _Hard_lock + 1, _LkN);
			std::this_thread::yield();

			return _Failed;
		}

		template<class _Ty>
		void _Lock_nonmember3(std::vector<_Ty>& _LkN) {
			// lock 3 or more locks, without deadlock
			int _Hard_lock = 0;
			while (_Hard_lock != -1) {
				_Hard_lock = _Lock_attempt(_Hard_lock, _LkN);
			}
		}
					
		template <class _Lock0, class _Lock1>
		bool _Lock_attempt_small2(_Lock0& _Lk0, _Lock1& _Lk1)
		{
			// attempt to lock 2 locks, by first locking _Lk0, and then trying to lock _Lk1 returns whether to try again
			_Lock_ref(_Lk0);
			try {
				if (_Try_lock_ref(_Lk1))
					return false;
			}
			catch (...) {
				_Unlock_ref(_Lk0);
				throw;
			}

			_Unlock_ref(_Lk0);
			std::this_thread::yield();
			return true;
		}

		template <class _Lock0, class _Lock1>
		void _Lock_nonmember2(_Lock0& _Lk0, _Lock1& _Lk1)
		{
			// lock 2 locks, without deadlock, special case for better codegen and reduced metaprogramming for common case
			while (_Lock_attempt_small2(_Lk0, _Lk1) && _Lock_attempt_small2(_Lk1, _Lk0)) { // keep trying
			}
		}

		template<class _Ty>
		void _Lock_range(std::vector<_Ty>& lockes)
		{
			if (lockes.size() == 0)
			{
			}
			else if (lockes.size() == 1)
			{
				_Lock_ref(lockes[0]);
			}
			else if (lockes.size() == 2)
			{
				_Lock_nonmember2(lockes[0], lockes[1]);
			}
			else
			{
				_Lock_nonmember3(lockes);
			}
		}
	}

	template<class _Ty>
	class scoped_lock_range { // class with destructor that unlocks mutexes
	public:
		explicit scoped_lock_range(std::vector<_Ty>& locks_)
			: _LkN(locks_)
		{
			detail::_Lock_range(locks_);
		}

		explicit scoped_lock_range(std::adopt_lock_t, std::vector<_Ty>& locks_)
			: _LkN(locks_)
		{ // construct but don't lock
		}

		~scoped_lock_range() noexcept
		{
			detail::_Unlock_locks(0, (int)_LkN.size(), _LkN);
		}

		scoped_lock_range(const scoped_lock_range&) = delete;
		scoped_lock_range& operator=(const scoped_lock_range&) = delete;
	private:
		std::vector<_Ty>& _LkN;
	};
}
