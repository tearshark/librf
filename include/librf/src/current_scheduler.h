#pragma once

namespace librf
{
	/**
	 * @brief 获得本协程绑定的调度器的可等待对象。
	 */
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
#ifdef DOXYGEN_SKIP_PROPERTY
	public:
		/**
		 * @brief 获得当前协程绑定的调度器。
		 * @details 立即返回，没有协程切换和等待。\n
		 * 推荐使用 librf_current_scheduler() 宏替代 co_await get_current_scheduler()。
		 * @return [co_await] scheduler_t*
		 * @note 本函数是librf名字空间下的全局函数。由于doxygen使用上的问题，将之归纳到 get_current_scheduler_awaitor 类下。
		 */
		static get_current_scheduler_awaitor get_current_scheduler() noexcept;

		/**
		 * @brief 获得当前协程绑定的调度器。
		 * @details 立即返回，没有协程切换和等待。\n
		 * 这是一条宏函数，等同于 co_await get_current_scheduler()。
		 * @return scheduler_t*
		 * @note 由于doxygen使用上的问题，将之归纳到 get_current_scheduler_awaitor 类下。
		 */
		static scheduler_t* librf_current_scheduler() noexcept;
#endif	//DOXYGEN_SKIP_PROPERTY
	};

	/**
	 * @brief 获得当前协程绑定的调度器。
	 * @details 立即返回，没有协程切换和等待。\n
	 * 推荐使用 librf_current_scheduler() 宏替代 co_await get_current_scheduler()。
	 * @return [co_await] scheduler_t*
	 */
	inline get_current_scheduler_awaitor get_current_scheduler() noexcept
	{
		return {};
	}


	/**
	 * @brief 获得本协程绑定的跟state指针的可等待对象。
	 */
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
#ifdef DOXYGEN_SKIP_PROPERTY
	public:
		/**
		 * @brief 获得当前协程的跟state指针。
		 * @details 立即返回，没有协程切换和等待。\n
		 * 推荐使用 librf_root_state() 宏替代 co_await get_root_state()。
		 * @return [co_await] state_base_t*
		 * @note 本函数是librf名字空间下的全局函数。由于doxygen使用上的问题，将之归纳到 get_root_state_awaitor 类下。
		 */
		static get_root_state_awaitor get_root_state() noexcept;

		/**
		 * @brief 获得当前协程的跟state指针。
		 * @details 立即返回，没有协程切换和等待。
		 * 这是一条宏函数，等同于 co_await get_root_state()。
		 * @return state_base_t*
		 * @note 由于doxygen使用上的问题，将之归纳到 get_root_state_awaitor 类下。
		 */
		static state_base_t* librf_root_state() noexcept;
#endif	//DOXYGEN_SKIP_PROPERTY
	};

	/**
	 * @brief 获得当前协程的跟state指针。
	 * @details 立即返回，没有协程切换和等待。
	 * 推荐使用 librf_root_state() 宏替代 co_await get_root_state()。
	 * @return [co_await] state_base_t*
	 */
	inline get_root_state_awaitor get_root_state() noexcept
	{
		return {};
	}

	/**
	 * @brief 获得本协程的task_t对象。
	 */
	struct get_current_task_awaitor
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
			state_base_t * state = parent->get_root();
			scheduler_t* sch = state->get_scheduler();

			this->_task = sch->find_task(state);

			return false;
		}
		task_t* await_resume() const noexcept
		{
			return _task;
		}
	private:
		task_t* _task;
#ifdef DOXYGEN_SKIP_PROPERTY
	public:
		/**
		 * @brief 获得当前协程的task_t指针。
		 * @details 立即返回，没有协程切换和等待。
		 * 推荐使用 librf_current_task() 宏替代 co_await get_current_task()。
		 * @return [co_await] task_t*
		 * @note 本函数是librf名字空间下的全局函数。由于doxygen使用上的问题，将之归纳到 get_current_task_awaitor 类下。
		 */
		static get_root_state_awaitor get_current_task() noexcept;

		/**
		 * @brief 获得当前协程的task_t指针。
		 * @details 立即返回，没有协程切换和等待。
		 * 这是一条宏函数，等同于 co_await get_current_task()。
		 * @return task_t*
		 * @note 由于doxygen使用上的问题，将之归纳到 get_current_task_awaitor 类下。
		 */
		static task_t* librf_current_task() noexcept;
#endif	//DOXYGEN_SKIP_PROPERTY
	};

	/**
	 * @brief 获得当前协程的task_t指针。
	 * @details 立即返回，没有协程切换和等待。
	 * 推荐使用 librf_current_task() 宏替代 co_await get_current_task()。
	 * @return [co_await] task_t*
	 */
	inline get_current_task_awaitor get_current_task() noexcept
	{
		return {};
	}
}
