#pragma once

RESUMEF_NS
{
	struct switch_scheduler_t
	{
		switch_scheduler_t(scheduler_t* sch)
			:_scheduler(sch) {}
		switch_scheduler_t(const switch_scheduler_t&) = default;
		switch_scheduler_t(switch_scheduler_t&&) = default;

		switch_scheduler_t& operator = (const switch_scheduler_t&) = default;
		switch_scheduler_t& operator = (switch_scheduler_t&&) = default;

		bool await_ready()
		{
			return false;
		}

		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		void await_suspend(coroutine_handle<_PromiseT> handler)
		{
			_PromiseT& promise = handler.promise();

			auto* sptr = promise.get_state();
			sptr->switch_scheduler_await_suspend(_scheduler, handler);
		}

		void await_resume()
		{
		}
	private:
		scheduler_t* _scheduler;
	};

	inline switch_scheduler_t via(scheduler_t* sch)
	{
		return { sch };
	}
}
