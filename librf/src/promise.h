#pragma once

#pragma push_macro("new")
#undef new

#ifndef DOXYGEN_SKIP_PROPERTY
namespace resumef
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

		promise_impl_t() noexcept {}
		promise_impl_t(promise_impl_t&& _Right) noexcept = default;
		promise_impl_t& operator = (promise_impl_t&& _Right) noexcept = default;
		promise_impl_t(const promise_impl_t&) = delete;
		promise_impl_t& operator = (const promise_impl_t&) = delete;

		auto get_state() noexcept->state_type*;

		suspend_on_initial initial_suspend() noexcept;
		suspend_on_final final_suspend() noexcept;
		template <typename _Uty>
		_Uty&& await_transform(_Uty&& _Whatever) noexcept;
		void set_exception(std::exception_ptr e);
#if defined(__clang__) || defined(__GNUC__)
		void unhandled_exception();		//If the coroutine ends with an uncaught exception, it performs the following: 
#endif
		future_type get_return_object() noexcept;
		void cancellation_requested() noexcept;

		using _Alloc_char = std::allocator<char>;
		void* operator new(size_t _Size);
		void operator delete(void* _Ptr, size_t _Size);
#if !RESUMEF_INLINE_STATE
	private:
		counted_ptr<state_type> _state = state_future_t::_Alloc_state<state_type>(false);
#endif
	};

	template<class _Ty>
	struct promise_t final : public promise_impl_t<_Ty>
	{
		using typename promise_impl_t<_Ty>::value_type;
		using promise_impl_t<_Ty>::get_return_object;

		template<class U>
		void return_value(U&& val);	//co_return val
		template<class U>
		suspend_always yield_value(U&& val);
	};

	template<class _Ty>
	struct promise_t<_Ty&> final : public promise_impl_t<_Ty&>
	{
		using typename promise_impl_t<_Ty&>::value_type;
		using promise_impl_t<_Ty&>::get_return_object;

		void return_value(_Ty& val);	//co_return val
		suspend_always yield_value(_Ty& val);
	};

	template<>
	struct promise_t<void> final : public promise_impl_t<void>
	{
		using promise_impl_t<void>::get_return_object;

		void return_void();			//co_return;
		suspend_always yield_value();
	};
#endif	//DOXYGEN_SKIP_PROPERTY
}

#pragma pop_macro("new")
