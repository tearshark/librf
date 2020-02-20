#pragma once

RESUMEF_NS
{
	struct suspend_on_initial
	{
		state_future_t* _state;

		inline bool await_ready() noexcept
		{
			return false;
		}
		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		inline void await_suspend(coroutine_handle<_PromiseT> handler) noexcept
		{
			_state->promise_initial_suspend(handler);
		}
		inline void await_resume() noexcept
		{
			_state->promise_await_resume();
		}
	};
	struct suspend_on_final
	{
		state_future_t* _state;

		inline bool await_ready() noexcept
		{
			return false;
		}
		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		inline void await_suspend(coroutine_handle<_PromiseT> handler) noexcept
		{
			_state->promise_final_suspend(handler);
		}
		inline void await_resume() noexcept
		{
			_state->promise_await_resume();
		}
	};

	template <typename _Ty>
	inline suspend_on_initial promise_impl_t<_Ty>::initial_suspend() noexcept
	{
        return { this->get_state() };
	}

	template <typename _Ty>
	inline suspend_on_final promise_impl_t<_Ty>::final_suspend() noexcept
	{
        return { this->get_state() };
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
		std::terminate();
	}
#endif

	template <typename _Ty>
	inline future_t<_Ty> promise_impl_t<_Ty>::get_return_object()
	{
        return { this->get_state() };
	}

	template <typename _Ty>
	inline void promise_impl_t<_Ty>::cancellation_requested()
	{

	}


	template<class _Ty>
	inline void promise_t<_Ty>::return_value(value_type val)
	{
        this->get_state()->set_value(std::move(val));
	}

	template<class _Ty>
	inline void promise_t<_Ty>::yield_value(value_type val)
	{
        this->get_state()->promise_yield_value(this, std::move(val));
	}

	inline void promise_t<void>::return_void()
	{
        this->get_state()->set_value();
	}

	inline void promise_t<void>::yield_value()
	{
        this->get_state()->promise_yield_value(this);
	}

}

