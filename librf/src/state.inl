#pragma once

namespace resumef
{
	template<class _PromiseT, typename _Enable>
	void state_future_t::promise_initial_suspend(coroutine_handle<_PromiseT> handler)
	{
		_PromiseT& promise = handler.promise();

		state_base_t* parent_state = promise._state.get();
		(void)parent_state;
		assert(this == parent_state);
		assert(this->_scheduler == nullptr);
		assert(this->_coro == nullptr);
		this->_initor = handler;
	}

	inline void state_future_t::promise_await_resume()
	{
	}

	template<class _PromiseT, typename _Enable>
	void state_future_t::promise_final_suspend(coroutine_handle<_PromiseT> handler)
	{
		scoped_lock<lock_type> __guard(this->_mtx);

		_PromiseT& promise = handler.promise();

		state_base_t* parent_state = promise._state.get();
		(void)parent_state;
		assert(this == parent_state);

		scheduler_t* sch = this->get_scheduler();
		assert(sch != nullptr);
		sch->del_final(this);
	}

	template<class _PromiseT, typename _Enable>
	void state_future_t::future_await_suspend(coroutine_handle<_PromiseT> handler)
	{
		scoped_lock<lock_type> __guard(this->_mtx);

		_PromiseT& promise = handler.promise();

		auto* parent_state = promise._state.get();
		scheduler_t* sch = parent_state->get_scheduler();
		if (this != parent_state)
		{
			this->_parent = parent_state;
			this->_scheduler = sch;
		}

		if (this->_coro == nullptr)
			this->_coro = handler;
		if (sch != nullptr)
			sch->add_await(this);
	}

	template<class _PromiseT, typename _Enable >
	void state_t<void>::promise_yield_value(_PromiseT* promise)
	{
		scoped_lock<lock_type> __guard(this->_mtx);

		this->_has_value = true;

		coroutine_handle<_PromiseT> handler = coroutine_handle<_PromiseT>::from_promise(*promise);
		if (!handler.done())
		{
			if (this->_coro == nullptr)
				this->_coro = handler;
			scheduler_t* sch = this->get_scheduler();
			if (sch != nullptr)
				sch->add_generator(this);
		}
	}

	template<typename _Ty>
	template<class _PromiseT, typename _Enable >
	void state_t<_Ty>::promise_yield_value(_PromiseT* promise, _Ty val)
	{
		scoped_lock<lock_type> __guard(this->_mtx);

		this->_value = std::move(val);

		coroutine_handle<_PromiseT> handler = coroutine_handle<_PromiseT>::from_promise(*promise);
		if (!handler.done())
		{
			if (this->_coro == nullptr)
				this->_coro = handler;
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
		return std::move(this->_value.value());
	}

	template<typename _Ty>
	void state_t<_Ty>::set_value(value_type val)
	{
		scoped_lock<lock_type> __guard(this->_mtx);

		this->_value = std::move(val);
		scheduler_t* sch = this->get_scheduler();
		if (sch != nullptr)
			sch->add_ready(this);
	}
}

