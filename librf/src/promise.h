#pragma once
#include "state.h"

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
		using lock_type = typename state_type::lock_type;

		counted_ptr<state_type> _state = make_counted<state_type>(false);

		promise_impl_t() {}
		promise_impl_t(promise_impl_t&& _Right) noexcept = default;
		promise_impl_t& operator = (promise_impl_t&& _Right) noexcept = default;
		promise_impl_t(const promise_impl_t&) = delete;
		promise_impl_t& operator = (const promise_impl_t&) = delete;

		suspend_on_initial initial_suspend() noexcept;
		suspend_on_final final_suspend() noexcept;
		void set_exception(std::exception_ptr e);
#ifdef __clang__
		void unhandled_exception();
#endif
		future_type get_return_object();
		void cancellation_requested();
	};

	template<class _Ty>
	struct promise_t : public promise_impl_t<_Ty>
	{
		using typename promise_impl_t<_Ty>::value_type;
		using promise_impl_t<_Ty>::_state;

		void return_value(value_type val);
		void yield_value(value_type val);
	};

	template<>
	struct promise_t<void> : public promise_impl_t<void>
	{
		using promise_impl_t<void>::_state;

		void return_void();
		void yield_value();
	};

}

