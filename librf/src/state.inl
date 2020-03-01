
RESUMEF_NS
{
	template<class _PromiseT, typename _Enable>
	void state_future_t::promise_initial_suspend(coroutine_handle<_PromiseT> handler)
	{
		assert(this->_scheduler == nullptr);
		assert(!this->_coro);

		this->_initor = handler;
		this->_is_initor = initor_type::Initial;
	}

	template<class _PromiseT, typename _Enable>
	void state_future_t::promise_final_suspend(coroutine_handle<_PromiseT> handler)
	{
		{
			scoped_lock<lock_type> __guard(this->_mtx);

			this->_initor = handler;
			this->_is_initor = initor_type::Final;
		}

		scheduler_t* sch = this->get_scheduler();
		assert(sch != nullptr);
		sch->del_final(this);
	}

	template<class _PromiseT, typename _Enable>
	void state_future_t::future_await_suspend(coroutine_handle<_PromiseT> handler)
	{
		_PromiseT& promise = handler.promise();
		auto* parent_state = promise.get_state();
		scheduler_t* sch = parent_state->get_scheduler();

		{
			scoped_lock<lock_type> __guard(this->_mtx);

			if (this != parent_state)
			{
				this->_parent = parent_state;
				this->_scheduler = sch;
			}

			if (!this->_coro)
				this->_coro = handler;
		}

		if (sch != nullptr)
			sch->add_await(this);
	}

	template<class _PromiseT, typename _Enable >
	void state_t<void>::promise_yield_value(_PromiseT* promise)
	{
		coroutine_handle<_PromiseT> handler = coroutine_handle<_PromiseT>::from_promise(*promise);

		{
			scoped_lock<lock_type> __guard(this->_mtx);

			if (!handler.done())
			{
				if (!this->_coro)
					this->_coro = handler;
			}

			this->_has_value = true;
		}

		if (!handler.done())
		{
			scheduler_t* sch = this->get_scheduler();
			if (sch != nullptr)
				sch->add_generator(this);
		}
	}

	template<typename _Ty>
	template<class _PromiseT, typename U, typename _Enable >
	void state_t<_Ty>::promise_yield_value(_PromiseT* promise, U&& val)
	{
		coroutine_handle<_PromiseT> handler = coroutine_handle<_PromiseT>::from_promise(*promise);

		{
			scoped_lock<lock_type> __guard(this->_mtx);

			if (!handler.done())
			{
				if (this->_coro == nullptr)
					this->_coro = handler;
			}

			if (this->_has_value)
			{
				*this->cast_value_ptr() = std::forward<U>(val);
			}
			else
			{
				new (this->cast_value_ptr()) value_type(std::forward<U>(val));
				this->_has_value = true;
			}
		}

		if (!handler.done())
		{
			scheduler_t* sch = this->get_scheduler();
			if (sch != nullptr)
				sch->add_generator(this);
		}
	}

	template<typename _Ty>
	auto state_t<_Ty>::future_await_resume() -> value_type
	{
		scoped_lock<lock_type> __guard(this->_mtx);
		if (this->_exception)
			std::rethrow_exception(std::move(this->_exception));
		if (!this->_has_value)
			std::rethrow_exception(std::make_exception_ptr(future_exception{error_code::not_ready}));

		return std::move(*this->cast_value_ptr());
	}

	template<typename _Ty>
	template<typename U>
	void state_t<_Ty>::set_value(U&& val)
	{
		{
			scoped_lock<lock_type> __guard(this->_mtx);

			if (this->_has_value)
			{
				*this->cast_value_ptr() = std::forward<U>(val);
			}
			else
			{
				new (this->cast_value_ptr()) value_type(std::forward<U>(val));
				this->_has_value = true;
			}
		}

		scheduler_t* sch = this->get_scheduler();
		if (sch != nullptr)
			sch->add_ready(this);
	}
}

