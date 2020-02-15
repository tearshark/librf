#pragma once

namespace resumef
{
	template<class _PromiseT, typename _Enable>
	inline void state_base_t::future_await_suspend(coroutine_handle<_PromiseT> handler)
	{
		scoped_lock<lock_type> __guard(this->_mtx);

		_PromiseT& promise = handler.promise();

		state_base_t* parent_state = promise._state.get();
		this->_parent = parent_state;
		scheduler_t* sch = parent_state->_scheduler;
		sch->add_await(this, handler);
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
		if (this->_scheduler != nullptr)
			this->_scheduler->add_ready(this);
	}
}

