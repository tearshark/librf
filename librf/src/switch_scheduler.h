#pragma once

RESUMEF_NS
{
	struct switch_scheduler_awaitor
	{
		switch_scheduler_awaitor(scheduler_t* sch)
			:_scheduler(sch) {}
		switch_scheduler_awaitor(const switch_scheduler_awaitor&) = default;
		switch_scheduler_awaitor(switch_scheduler_awaitor&&) = default;

		switch_scheduler_awaitor& operator = (const switch_scheduler_awaitor&) = default;
		switch_scheduler_awaitor& operator = (switch_scheduler_awaitor&&) = default;

		bool await_ready() noexcept
		{
			return false;
		}

		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		bool await_suspend(coroutine_handle<_PromiseT> handler)
		{
			_PromiseT& promise = handler.promise();

			auto* sptr = promise.get_state();
			return sptr->switch_scheduler_await_suspend(_scheduler, handler);
		}

		void await_resume() noexcept
		{
		}
	private:
		scheduler_t* _scheduler;
	};

	inline switch_scheduler_awaitor operator co_await(scheduler_t& sch)
	{
		return { &sch };
	}
}
