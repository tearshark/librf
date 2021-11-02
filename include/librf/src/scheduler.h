#pragma once

namespace librf
{
	/**
	 * @brief 获得当前线程下的调度器。
	 */
	LIBRF_API scheduler_t* this_scheduler();

	/**
	 * @brief 协程调度器。
	 * @details librf的设计原则之一，就是要将协程绑定在固定的调度器里执行。
	 * 通过控制调度器运行的线程和时机，从而控制协程所在的线程和运行时机。
	 */
	struct scheduler_t : public std::enable_shared_from_this<scheduler_t>
	{
	private:
		using state_sptr = counted_ptr<state_base_t>;
		using state_vector = std::vector<state_sptr>;
		using lock_type = spinlock;
		using task_dictionary_type = std::unordered_map<state_base_t*, std::unique_ptr<task_t>>;

#if !RESUMEF_DISABLE_MULT_THREAD
		mutable spinlock _lock_running;
#endif
		state_vector _runing_states;
		state_vector _cached_states;

#if !RESUMEF_DISABLE_MULT_THREAD
		mutable spinlock _lock_ready;
#endif
		task_dictionary_type _ready_task;

		timer_mgr_ptr _timer;

		LIBRF_API task_t* new_task(task_t* task);
		//void cancel_all_task_();
	public:
		/**
		 * @brief 运行一批准备妥当的协程。
		 * @details 这是协程调度器提供的主要接口。同一个调度器非线程安全，不可重入。\n
		 * 调用者要保证此函数始终在同一个线程里调用。
		 */
		LIBRF_API bool run_one_batch();

		/**
		 * @brief 循环运行所有的协程，直到所有协程都运行完成。
		 * @details 通常用于测试代码。
		 */
		LIBRF_API void run_until_notask();

		//void break_all();

		/**
		 * @brief 将一个协程加入到调度器里开始运行。
		 * @details 推荐使用go或者GO这两个宏来启动协程。\n
		 * go用于启动future_t<>/generator_t<>；\n
		 * GO用于启动一个所有变量按值捕获的lambda。
		 * @param coro 协程对象。future_t<>，generator_t<>，或者一个调用后返回future_t<>/generator_t<>的函数对象。
		 * @retval task_t* 返回代表一个新协程的协程任务类。\n
		 */
		template<class _Ty>
		requires(traits::is_callable_v<_Ty> || traits::is_future_v<_Ty> || traits::is_generator_v<_Ty>)
		task_t* operator + (_Ty&& coro)
		{
			if constexpr (traits::is_callable_v<_Ty>)
				return new_task(new task_ctx_impl_t<_Ty>(coro));
			else
				return new_task(new task_impl_t<_Ty>(coro));
		}

		/**
		 * @brief 判断所有协程是否运行完毕。
		 * @retval bool 以下条件全部满足，返回true：\n
		 * 1、所有协程运行完毕\n
		 * 2、没有正在准备执行的state\n
		 * 3、定时管理器的empty()返回true。
		 */
		bool empty() const
		{
#if !RESUMEF_DISABLE_MULT_THREAD
			scoped_lock<spinlock, spinlock> __guard(_lock_ready, _lock_running);
#endif
			return _ready_task.empty() && _runing_states.empty() && _timer->empty();
		}

		/**
		 * @brief 获得定时管理器。
		 */
		timer_manager* timer() const noexcept
		{
			return _timer.get();
		}

#ifndef DOXYGEN_SKIP_PROPERTY
		void add_generator(state_base_t* sptr);
		void del_final(state_base_t* sptr);
		LIBRF_API std::unique_ptr<task_t> del_switch(state_base_t* sptr);
		void add_switch(std::unique_ptr<task_t> task);
		task_t* find_task(state_base_t* sptr) const noexcept;

		friend struct local_scheduler_t;
	protected:
		LIBRF_API scheduler_t();
	public:
		LIBRF_API ~scheduler_t();

		scheduler_t(scheduler_t&& right_) = delete;
		scheduler_t& operator = (scheduler_t&& right_) = delete;
		scheduler_t(const scheduler_t&) = delete;
		scheduler_t& operator = (const scheduler_t&) = delete;

		LIBRF_API static scheduler_t g_scheduler;
#endif	//DOXYGEN_SKIP_PROPERTY
	};

	/**
	 * @brief 创建一个线程相关的调度器。
	 * @details 如果线程之前已经创建了调度器，则第一个调度器会跟线程绑定，此后local_scheduler_t不会创建更多的调度器。\n
	 * 否则，local_scheduler_t会创建一个调度器，并绑定到创建local_scheduler_t的线程上。\n
	 * 如果local_scheduler_t成功创建了一个调度器，则在local_scheduler_t生命周期结束后，会销毁创建的调度器，并解绑线程。\n
	 * 典型用法，是在非主线程里，开始运行协程之前，申明一个local_scheduler_t的局部变量。
	 */
	struct local_scheduler_t
	{
		/**
		 * @brief 尽可能的创建一个线程相关的调度器。
		 */
		LIBRF_API local_scheduler_t();

		/**
		 * @brief 将指定的调度器绑定到当前线程上。
		 */
		LIBRF_API local_scheduler_t(scheduler_t & sch) noexcept;

		/**
		 * @brief 如果当前线程绑定的调度器由local_scheduler_t所创建，则会销毁调度器，并解绑线程。
		 */
		LIBRF_API ~local_scheduler_t();

		local_scheduler_t(local_scheduler_t&& right_) = delete;
		local_scheduler_t& operator = (local_scheduler_t&& right_) = delete;
		local_scheduler_t(const local_scheduler_t&) = delete;
		local_scheduler_t& operator = (const local_scheduler_t&) = delete;
	private:
		scheduler_t* _scheduler_ptr;
	};

	inline void scheduler_t::add_generator(state_base_t* sptr)
	{
		assert(sptr != nullptr);

#if !RESUMEF_DISABLE_MULT_THREAD
		scoped_lock<spinlock> __guard(_lock_running);
#endif
		_runing_states.emplace_back(sptr);
	}

	inline void scheduler_t::del_final(state_base_t* sptr)
	{
#if !RESUMEF_DISABLE_MULT_THREAD
		scoped_lock<spinlock> __guard(_lock_ready);
#endif
		this->_ready_task.erase(sptr);
	}

	inline void scheduler_t::add_switch(std::unique_ptr<task_t> task)
	{
		state_base_t* sptr = task->_state.get();

#if !RESUMEF_DISABLE_MULT_THREAD
		scoped_lock<spinlock> __guard(_lock_ready);
#endif
		this->_ready_task.emplace(sptr, std::move(task));
	}

	inline task_t* scheduler_t::find_task(state_base_t* sptr) const noexcept
	{
#if !RESUMEF_DISABLE_MULT_THREAD
		scoped_lock<spinlock> __guard(_lock_ready);
#endif

		auto iter = this->_ready_task.find(sptr);
		if (iter != this->_ready_task.end())
			return iter->second.get();
		return nullptr;
	}
}
