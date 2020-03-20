
struct LOCK_ASSEMBLE_NAME(lock_impl)
{
	// FUNCTION TEMPLATE _Unlock_locks
	template<_LockAssembleT _LA>
	static void _Unlock_locks(int _First, int _Last, _LA& _LkN) noexcept /* terminates */
	{
		for (; _First != _Last; ++_First) {
			_LkN._Unlock_ref(_LkN[_First]);
		}
	}

	// FUNCTION TEMPLATE try_lock
	template<_LockAssembleT _LA>
	static auto _Try_lock_range(const int _First, const int _Last, _LA& _LkN)
		->decltype(_LkN._ReturnValue<int>(0))
	{
		int _Next = _First;
		try {
			for (; _Next != _Last; ++_Next)
			{
				if (!LOCK_ASSEMBLE_AWAIT(_LkN._Try_lock_ref(_LkN[_Next])))
				{ // try_lock failed, backout
					_Unlock_locks(_First, _Next, _LkN);
					LOCK_ASSEMBLE_RETURN(_Next);
				}
			}
		}
		catch (...) {
			_Unlock_locks(_First, _Next, _LkN);
			throw;
		}

		LOCK_ASSEMBLE_RETURN(-1);
	}

	// FUNCTION TEMPLATE lock
	template<_LockAssembleT _LA>
	static auto _Lock_attempt(const int _Hard_lock, _LA& _LkN)
		->decltype(_LkN._ReturnValue<int>(0))
	{
		// attempt to lock 3 or more locks, starting by locking _LkN[_Hard_lock] and trying to lock the rest
		LOCK_ASSEMBLE_AWAIT(_LkN._Lock_ref(_LkN[_Hard_lock]));
		int _Failed = -1;
		int _Backout_start = _Hard_lock; // that is, unlock _Hard_lock

		try {
			_Failed = LOCK_ASSEMBLE_AWAIT(_Try_lock_range(0, _Hard_lock, _LkN));
			if (_Failed == -1)
			{
				_Backout_start = 0; // that is, unlock [0, _Hard_lock] if the next throws
				_Failed = LOCK_ASSEMBLE_AWAIT(_Try_lock_range(_Hard_lock + 1, (int)_LkN.size(), _LkN));
				if (_Failed == -1) { // we got all the locks
					LOCK_ASSEMBLE_RETURN(-1);
				}
			}
		}
		catch (...) {
			_Unlock_locks(_Backout_start, _Hard_lock + 1, _LkN);
			throw;
		}

		// we didn't get all the locks, backout
		_Unlock_locks(_Backout_start, _Hard_lock + 1, _LkN);
		LOCK_ASSEMBLE_AWAIT(_LkN._Yield());

		LOCK_ASSEMBLE_RETURN(_Failed);
	}

	template<_LockAssembleT _LA>
	static auto _Lock_nonmember3(_LA& _LkN) ->decltype(_LkN._ReturnValue())
	{
		// lock 3 or more locks, without deadlock
		int _Hard_lock = 0;
		while (_Hard_lock != -1) {
			_Hard_lock = LOCK_ASSEMBLE_AWAIT(_Lock_attempt(_Hard_lock, _LkN));
		}
	}
					
	template<_LockAssembleT _LA>
	static auto _Lock_attempt_small2(_LA& _LkN, const int _Idx0, const int _Idx1)
		->decltype(_LkN._ReturnValue<bool>(false))
	{
		// attempt to lock 2 locks, by first locking _Lk0, and then trying to lock _Lk1 returns whether to try again
		LOCK_ASSEMBLE_AWAIT(_LkN._Lock_ref(_LkN[_Idx0]));
		try {
			if (LOCK_ASSEMBLE_AWAIT(_LkN._Try_lock_ref(_LkN[_Idx1])))
				LOCK_ASSEMBLE_RETURN(false);
		}
		catch (...) {
			_LkN._Unlock_ref(_LkN[_Idx0]);
			throw;
		}

		_LkN._Unlock_ref(_LkN[_Idx0]);
		LOCK_ASSEMBLE_AWAIT(_LkN._Yield());

		LOCK_ASSEMBLE_RETURN(true);
	}

	template<_LockAssembleT _LA>
	static auto _Lock_nonmember2(_LA& _LkN) ->decltype(_LkN._ReturnValue())
	{
		// lock 2 locks, without deadlock, special case for better codegen and reduced metaprogramming for common case
		while (LOCK_ASSEMBLE_AWAIT(_Lock_attempt_small2(_LkN, 0, 1)) && 
			LOCK_ASSEMBLE_AWAIT(_Lock_attempt_small2(_LkN, 1, 0)))
		{ // keep trying
		}
	}

	template<_LockAssembleT _LA>
	static auto _Lock_range(_LA& lockes) ->decltype(lockes._ReturnValue())
	{
		if (lockes.size() == 0)
		{
		}
		else if (lockes.size() == 1)
		{
			LOCK_ASSEMBLE_AWAIT(lockes._Lock_ref(lockes[0]));
		}
		else if (lockes.size() == 2)
		{
			LOCK_ASSEMBLE_AWAIT(_Lock_nonmember2(lockes));
		}
		else
		{
			LOCK_ASSEMBLE_AWAIT(_Lock_nonmember3(lockes));
		}
	}
};

