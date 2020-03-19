#pragma once

RESUMEF_NS
{
	struct get_current_scheduler_awaitor
	{
		bool await_ready() const noexcept
		{
			return false;
		}
		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		bool await_suspend(coroutine_handle<_PromiseT> handler)
		{
			_PromiseT& promise = handler.promise();
			auto* state = promise.get_state();
			this->_scheduler = state->get_scheduler();

			return false;
		}
		scheduler_t* await_resume() const noexcept
		{
			return _scheduler;
		}
	private:
		scheduler_t* _scheduler;
	};

	inline get_current_scheduler_awaitor get_current_scheduler()
	{
		return {};
	}


	struct get_root_state_awaitor
	{
		bool await_ready() const noexcept
		{
			return false;
		}
		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		bool await_suspend(coroutine_handle<_PromiseT> handler)
		{
			_PromiseT& promise = handler.promise();
			auto* parent = promise.get_state();
			this->_state = parent->get_root();

			return false;
		}
		state_base_t* await_resume() const noexcept
		{
			return _state;
		}
	private:
		state_base_t* _state;
	};

	inline get_root_state_awaitor get_root_state()
	{
		return {};
	}

}
