#pragma once

#pragma push_macro("new")
#undef new

RESUMEF_NS
{
	struct suspend_on_initial;
	struct suspend_on_final;

	template <typename _Ty>
	struct promise_impl_t
	{
		using value_type = _Ty;
		using state_type = state_t<value_type>;
		using promise_type = promise_t<value_type>;
		using future_type = future_t<value_type>;

		promise_impl_t(){}
		promise_impl_t(promise_impl_t&& _Right) noexcept = default;
		promise_impl_t& operator = (promise_impl_t&& _Right) noexcept = default;
		promise_impl_t(const promise_impl_t&) = delete;
		promise_impl_t& operator = (const promise_impl_t&) = delete;

		state_type* get_state()
		{
			size_t _State_size = _Align_size<state_type>();
			char* ptr = reinterpret_cast<char*>(this) - _State_size;
			return reinterpret_cast<state_type*>(ptr);
		}

		suspend_on_initial initial_suspend() noexcept;
		suspend_on_final final_suspend() noexcept;
		void set_exception(std::exception_ptr e);
#ifdef __clang__
		void unhandled_exception();		//If the coroutine ends with an uncaught exception, it performs the following: 
#endif
		future_type get_return_object();
		void cancellation_requested();

		using _Alloc_char = std::allocator<char>;
		void* operator new(size_t _Size)
		{
			size_t _State_size = _Align_size<state_type>();
			assert(_Size >= sizeof(uint32_t) && _Size < (std::numeric_limits<uint32_t>::max)() - sizeof(_State_size));

			_Alloc_char _Al;
			/*If allocation fails, the coroutine throws std::bad_alloc, 
			unless the Promise type defines the member function Promise::get_return_object_on_allocation_failure(). 
			If that member function is defined, allocation uses the nothrow form of operator new and on allocation failure, 
			the coroutine immediately returns the object obtained from Promise::get_return_object_on_allocation_failure() to the caller. 
			*/
			char* ptr = _Al.allocate(_Size + _State_size);
#if RESUMEF_DEBUG_COUNTER
			std::cout << "  future_promise::new, alloc size=" << (_Size + _State_size) << std::endl;
			std::cout << "  future_promise::new, alloc ptr=" << (void*)ptr << std::endl;
			std::cout << "  future_promise::new, return ptr=" << (void*)(ptr + _State_size) << std::endl;
#endif

			//在初始地址上构造state
			{
				state_type* st = new(ptr) state_type(_Size + _State_size);
				st->lock();
			}

			return ptr + _State_size;
		}

		void operator delete(void* _Ptr, size_t _Size)
		{
			(void)_Size;
			size_t _State_size = _Align_size<state_type>();
			assert(_Size >= sizeof(uint32_t) && _Size < (std::numeric_limits<uint32_t>::max)() - sizeof(_State_size));

			state_type* st = reinterpret_cast<state_type*>(static_cast<char*>(_Ptr) - _State_size);
			st->unlock();
		}
	};

	template<class _Ty>
	struct promise_t final : public promise_impl_t<_Ty>
	{
		using typename promise_impl_t<_Ty>::value_type;
		using promise_impl_t<_Ty>::get_return_object;

		template<class U>
		void return_value(U&& val);	//co_return val
		template<class U>
		void yield_value(U&& val);
	};

	template<>
	struct promise_t<void> final : public promise_impl_t<void>
	{
		using promise_impl_t<void>::get_return_object;

		void return_void();			//co_return;
		void yield_value();
	};

}

#pragma pop_macro("new")
