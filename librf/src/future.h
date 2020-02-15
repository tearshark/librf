
#pragma once

#include "state.h"

namespace resumef
{
	template<class _Ty>
	struct future_impl_t
	{
		using value_type = _Ty;
		using state_type = state_t<value_type>;
		using promise_type = promise_t<value_type>;
		using future_type = future_t<value_type>;

		counted_ptr<state_type> _state;

		future_impl_t(counted_ptr<state_type> _st)
			:_state(std::move(_st)) {}
		future_impl_t(const future_impl_t&) = default;
		future_impl_t(future_impl_t&&) = default;

		future_impl_t& operator = (const future_impl_t&) = default;
		future_impl_t& operator = (future_impl_t&&) = default;

		bool await_ready()
		{
			return _state->has_value();
		}

		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		void await_suspend(coroutine_handle<_PromiseT> handler)
		{
			_PromiseT& promise = handler.promise();
			scheduler_t* sch = promise._state->_scheduler;
			sch->add_await(_state.get(), handler);
		}
	};

	template<class _Ty>
	struct future_t : public future_impl_t<_Ty>
	{
		using future_impl_t::future_impl_t;
		using future_impl_t::_state;

		value_type await_resume()
		{
			if (_state->_exception)
				std::rethrow_exception(std::move(_state->_exception));
			return std::move(_state->_value.value());
		}
	};

	template<>
	struct future_t<void> : public future_impl_t<void>
	{
		using future_impl_t::future_impl_t;
		using future_impl_t::_state;

		void await_resume()
		{
			if (_state->_exception)
				std::rethrow_exception(std::move(_state->_exception));
		}
	};
}

