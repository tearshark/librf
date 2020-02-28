
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
		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
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
		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
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
	inline future_t<_Ty> promise_impl_t<_Ty>::get_return_object()
	{
        return { this->get_state() };
	}

	template <typename _Ty>
	inline void promise_impl_t<_Ty>::cancellation_requested()
	{

	}


	template<class _Ty>
	template<class U>
	inline void promise_t<_Ty>::return_value(U&& val)
	{
        this->get_state()->set_value(std::forward<U>(val));
	}

	template<class _Ty>
	template<class U>
	inline void promise_t<_Ty>::yield_value(U&& val)
	{
        this->get_state()->promise_yield_value(this, std::forward<U>(val));
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

