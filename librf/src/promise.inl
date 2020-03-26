
RESUMEF_NS
{
	/*
	Note: the awaiter object is part of coroutine state (as a temporary whose lifetime crosses a suspension point) 
	and is destroyed before the co_await expression finishes. 
	It can be used to maintain per-operation state as required by some async I/O APIs without resorting to additional heap allocations. 
	*/

	struct suspend_on_initial
	{
		inline bool await_ready() noexcept
		{
			return false;
		}
		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		inline void await_suspend(coroutine_handle<_PromiseT> handler) noexcept
		{
			_PromiseT& promise = handler.promise();
			auto* _state = promise.get_state();
			_state->promise_initial_suspend(handler);
		}
		inline void await_resume() noexcept
		{
		}
	};

	struct suspend_on_final
	{
		inline bool await_ready() noexcept
		{
			return false;
		}
		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		inline void await_suspend(coroutine_handle<_PromiseT> handler) noexcept
		{
			_PromiseT& promise = handler.promise();
			auto* _state = promise.get_state();
			_state->promise_final_suspend(handler);
		}
		inline void await_resume() noexcept
		{
		}
	};

	template <typename _Ty>
	inline suspend_on_initial promise_impl_t<_Ty>::initial_suspend() noexcept
	{
        return {};
	}

	template <typename _Ty>
	inline suspend_on_final promise_impl_t<_Ty>::final_suspend() noexcept
	{
        return {};
	}

	template <typename _Ty>
	template <typename _Uty>
	_Uty&& promise_impl_t<_Ty>::await_transform(_Uty&& _Whatever) noexcept
	{
		if constexpr (traits::has_state_v<_Uty>)
		{
			_Whatever._state->set_scheduler(get_state()->get_scheduler());
		}

		return std::forward<_Uty>(_Whatever);
	}

	template <typename _Ty>
	inline void promise_impl_t<_Ty>::set_exception(std::exception_ptr e)
	{
        this->get_state()->set_exception(std::move(e));
	}

#ifdef __clang__
	template <typename _Ty>
	inline void promise_impl_t<_Ty>::unhandled_exception()
	{
		this->get_state()->set_exception(std::current_exception());
	}
#endif

	template <typename _Ty>
	inline future_t<_Ty> promise_impl_t<_Ty>::get_return_object() noexcept
	{
        return { this->get_state() };
	}

	template <typename _Ty>
	inline void promise_impl_t<_Ty>::cancellation_requested() noexcept
	{

	}

	template <typename _Ty>
	auto promise_impl_t<_Ty>::get_state() noexcept -> state_type*
	{
#if RESUMEF_INLINE_STATE
		size_t _State_size = _Align_size<state_type>();
#if defined(__clang__)
		auto h = coroutine_handle<promise_type>::from_promise(*reinterpret_cast<promise_type *>(this));
		char* ptr = reinterpret_cast<char*>(h.address()) - _State_size;
		return reinterpret_cast<state_type*>(ptr);
#elif defined(_MSC_VER)
		char* ptr = reinterpret_cast<char*>(this) - _State_size;
		return reinterpret_cast<state_type*>(ptr);
#else
#error "Unknown compiler"
#endif
#else
		return _state.get();
#endif
	}

	template <typename _Ty>
	void* promise_impl_t<_Ty>::operator new(size_t _Size)
	{
		_Alloc_char _Al;
#if RESUMEF_INLINE_STATE
		size_t _State_size = _Align_size<state_type>();
		assert(_Size >= sizeof(uint32_t) && _Size < (std::numeric_limits<uint32_t>::max)() - sizeof(_State_size));

		/*If allocation fails, the coroutine throws std::bad_alloc,
		unless the Promise type defines the member function Promise::get_return_object_on_allocation_failure().
		If that member function is defined, allocation uses the nothrow form of operator new and on allocation failure,
		the coroutine immediately returns the object obtained from Promise::get_return_object_on_allocation_failure() to the caller.
		*/
		char* ptr = _Al.allocate(_Size + _State_size);
		char* _Rptr = ptr + _State_size;
#if RESUMEF_DEBUG_COUNTER
		std::cout << "  future_promise::new, alloc size=" << (_Size + _State_size) << std::endl;
		std::cout << "  future_promise::new, alloc ptr=" << (void*)ptr << std::endl;
		std::cout << "  future_promise::new, return ptr=" << (void*)_Rptr << std::endl;
#endif

		//在初始地址上构造state
		{
			state_type* st = state_future_t::_Construct<state_type>(ptr, _Size + _State_size);
			st->lock();
		}

		return _Rptr;
#else
		char* ptr = _Al.allocate(_Size);
#if RESUMEF_DEBUG_COUNTER
		std::cout << "  future_promise::new, alloc size=" << (_Size) << std::endl;
		std::cout << "  future_promise::new, alloc ptr=" << (void*)ptr << std::endl;
		std::cout << "  future_promise::new, return ptr=" << (void*)ptr << std::endl;
#endif
		return ptr;
#endif
	}

	template <typename _Ty>
	void promise_impl_t<_Ty>::operator delete(void* _Ptr, size_t _Size)
	{
#if RESUMEF_INLINE_STATE
		(void)_Size;
		size_t _State_size = _Align_size<state_type>();
		assert(_Size >= sizeof(uint32_t) && _Size < (std::numeric_limits<uint32_t>::max)() - sizeof(_State_size));

		state_type* st = reinterpret_cast<state_type*>(static_cast<char*>(_Ptr) - _State_size);
		st->unlock();
#else
		_Alloc_char _Al;
		return _Al.deallocate(reinterpret_cast<char*>(_Ptr), _Size);
#endif
	}

	template<class _Ty>
	template<class U>
	inline void promise_t<_Ty>::return_value(U&& val)
	{
        this->get_state()->set_value(std::forward<U>(val));
	}

	template<class _Ty>
	template<class U>
	inline suspend_always promise_t<_Ty>::yield_value(U&& val)
	{
        this->get_state()->promise_yield_value(this, std::forward<U>(val));
		return {};
	}

	template<class _Ty>
	inline void promise_t<_Ty&>::return_value(_Ty& val)
	{
		this->get_state()->set_value(val);
	}

	template<class _Ty>
	inline suspend_always promise_t<_Ty&>::yield_value(_Ty& val)
	{
		this->get_state()->promise_yield_value(this, val);
		return {};
	}

	inline void promise_t<void>::return_void()
	{
        this->get_state()->set_value();
	}

	inline suspend_always promise_t<void>::yield_value()
	{
        this->get_state()->promise_yield_value(this);
		return {};
	}
}

