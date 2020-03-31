#pragma once

namespace resumef
{
	/**
	 * @brief 切换协程的可等待对象。
	 */
	struct switch_scheduler_awaitor
	{
		using value_type = void;
		using state_type = state_t<value_type>;
		using promise_type = promise_t<value_type>;
		using lock_type = typename state_type::lock_type;

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

		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		bool await_suspend(coroutine_handle<_PromiseT> handler)
		{
			_PromiseT& promise = handler.promise();
			auto* sptr = promise.get_state();
			if (sptr->switch_scheduler_await_suspend(_scheduler))
			{
				counted_ptr<state_t<void>> _state = state_future_t::_Alloc_state<state_type>(true);
				_state->set_value();
				_state->future_await_suspend(handler);

				return true;
			}
			return false;
		}

		void await_resume() noexcept
		{
		}

	private:
		scheduler_t* _scheduler;
#ifdef DOXYGEN_SKIP_PROPERTY
	public:
		/**
		 * @brief 将本协程切换到指定调度器上运行。
		 * @details 由于调度器必然在某个线程里运行，故达到了切换到特定线程里运行的目的。\n
		 * 如果指定的协程就是本协程的调度器，则协程不暂停直接运行接下来的代码。
		 * 如果指定的协程不是本协程的调度器，则协程暂停后放入到目的协程的调度队列，等待下一次运行。
		 * @param sch 将要运行后续代码的协程
		 * @return [co_await] void
		 * @note 本函数是resumef名字空间下的全局函数。由于doxygen使用上的问题，将之归纳到 switch_scheduler_awaitor 类下。
		 */
		static switch_scheduler_awaitor via(scheduler_t& sch) noexcept;

		/**
		 * @brief 将本协程切换到指定调度器上运行。
		 * @details 由于调度器必然在某个线程里运行，故达到了切换到特定线程里运行的目的。\n
		 * 如果指定的协程就是本协程的调度器，则协程不暂停直接运行接下来的代码。
		 * 如果指定的协程不是本协程的调度器，则协程暂停后放入到目的协程的调度队列，等待下一次运行。
		 * @param sch 将要运行后续代码的协程
		 * @return [co_await] void
		 * @note 本函数是resumef名字空间下的全局函数。由于doxygen使用上的问题，将之归纳到 switch_scheduler_awaitor 类下。
		 */
		static switch_scheduler_awaitor via(scheduler_t* sch) noexcept;
#endif	//DOXYGEN_SKIP_PROPERTY
	};

	//由于跟when_all/when_any混用的时候，在clang上编译失败：
	//clang把scheduler_t判断成一个is_awaitable，且放弃选择when_all/any(scheduler_t& sch, ...)版本
	//故放弃这种用法
	//inline switch_scheduler_awaitor operator co_await(scheduler_t& sch) noexcept
	//{
	//	return { &sch };
	//}

	/**
	 * @brief 将本协程切换到指定调度器上运行。
	 * @details 由于调度器必然在某个线程里运行，故达到了切换到特定线程里运行的目的。\n
	 * 如果指定的协程就是本协程的调度器，则协程不暂停直接运行接下来的代码。
	 * 如果指定的协程不是本协程的调度器，则协程暂停后放入到目的协程的调度队列，等待下一次运行。
	 * @param sch 将要运行此后代码的协程
	 */
	inline switch_scheduler_awaitor via(scheduler_t& sch) noexcept
	{
		return { &sch };
	}

	/**
	 * @fn 将本协程切换到指定调度器上运行。
	 * @see 参考 via(scheduler_t&)版本。
	 */
	inline switch_scheduler_awaitor via(scheduler_t* sch) noexcept
	{
		return { sch };
	}

}
