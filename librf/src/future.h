
#pragma once

#include "state.h"

RESUMEF_NS
{
	template<class _Ty>
	struct future_t
	{
		using value_type = _Ty;
		using state_type = state_t<value_type>;
		using promise_type = promise_t<value_type>;
		using future_type = future_t<value_type>;
		using lock_type = typename state_type::lock_type;

		counted_ptr<state_type> _state;

		future_t(counted_ptr<state_type> _st)
			:_state(std::move(_st)) {}
		future_t(const future_t&) = default;
		future_t(future_t&&) = default;

		future_t& operator = (const future_t&) = default;
		future_t& operator = (future_t&&) = default;

		bool await_ready()
		{
			return _state->future_await_ready();
		}

		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		void await_suspend(coroutine_handle<_PromiseT> handler)
		{
			_state->future_await_suspend(handler);
		}

		value_type await_resume()
		{
			return _state->future_await_resume();
		}
	};
}

