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
		scheduler_t* _scheduler = nullptr;
	};

	inline get_current_scheduler_awaitor get_current_scheduler()
	{
		return {};
	}

}
