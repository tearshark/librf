
#pragma once

#pragma once

namespace resumef
{
	struct suspend_on_initial
	{
		counted_ptr<state_base_t> _state;

		inline bool await_ready() noexcept
		{
			return false;
		}
		inline void await_suspend(coroutine_handle<> handler) noexcept
		{
			_state->set_handler(handler);
		}
		inline void await_resume() noexcept
		{
		}
	};

	template <typename _Ty>
	inline auto promise_impl_t<_Ty>::initial_suspend() noexcept
	{
		return suspend_on_initial{ _state.get() };
	}

	template <typename _Ty>
	inline auto promise_impl_t<_Ty>::final_suspend() noexcept
	{
		_state->promise_final_suspend();
		return std::experimental::suspend_never{};
	}

	template <typename _Ty>
	inline void promise_impl_t<_Ty>::set_exception(std::exception_ptr e)
	{
		_state->set_exception(std::move(e));
	}

	template <typename _Ty>
	inline future_t<_Ty> promise_impl_t<_Ty>::get_return_object()
	{
		return { _state };
	}

	template <typename _Ty>
	inline void promise_impl_t<_Ty>::cancellation_requested()
	{

	}


	template<class _Ty>
	inline void promise_t<_Ty>::return_value(value_type val)
	{
		_state->set_value(std::move(val));
	}

	template<class _Ty>
	inline void promise_t<_Ty>::yield_value(value_type val)
	{
		_state->set_value(std::move(val));
	}

	inline void promise_t<void>::return_void()
	{
		_state->set_value();
	}

	inline void promise_t<void>::yield_value()
	{
		_state->set_value();
	}

}

