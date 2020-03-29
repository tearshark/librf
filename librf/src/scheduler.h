#pragma once

#ifndef DOXYGEN_SKIP_PROPERTY
RESUMEF_NS
{
	struct local_scheduler;
#endif	//DOXYGEN_SKIP_PROPERTY

	struct scheduler_t : public std::enable_shared_from_this<scheduler_t>
	{
	private:
		using state_sptr = counted_ptr<state_base_t>;
		using state_vector = std::vector<state_sptr>;
		using lock_type = spinlock;
		using task_dictionary_type = std::unordered_map<state_base_t*, std::unique_ptr<task_base_t>>;

		mutable spinlock _lock_running;
		state_vector _runing_states;
		state_vector _cached_states;

		mutable spinlock _lock_ready;
		task_dictionary_type _ready_task;

		timer_mgr_ptr _timer;

		void new_task(task_base_t* task);
		//void cancel_all_task_();
	public:
		void run_one_batch();
		void run_until_notask();
		void run();
		//void break_all();

		template<class _Ty 
#ifndef DOXYGEN_SKIP_PROPERTY
			COMMA_RESUMEF_ENABLE_IF(traits::is_callable_v<_Ty> || traits::is_future_v<_Ty> || traits::is_generator_v<_Ty>)
#endif	//DOXYGEN_SKIP_PROPERTY
		>
#ifndef DOXYGEN_SKIP_PROPERTY
		RESUMEF_REQUIRES(traits::is_callable_v<_Ty> || traits::is_future_v<_Ty> || traits::is_generator_v<_Ty>)
#endif	//DOXYGEN_SKIP_PROPERTY
		void operator + (_Ty&& t_)
		{
			if constexpr (traits::is_callable_v<_Ty>)
				new_task(new ctx_task_t<_Ty>(t_));
			else
				new_task(new task_t<_Ty>(t_));
		}

		bool empty() const
		{
			scoped_lock<spinlock, spinlock> __guard(_lock_ready, _lock_running);
			return _ready_task.empty() && _runing_states.empty() && _timer->empty();
		}

		timer_manager* timer() const noexcept
		{
			return _timer.get();
		}

#ifndef DOXYGEN_SKIP_PROPERTY
		void add_generator(state_base_t* sptr);
		void del_final(state_base_t* sptr);
		std::unique_ptr<task_base_t> del_switch(state_base_t* sptr);
		void add_switch(std::unique_ptr<task_base_t> task);

		friend struct local_scheduler;
	protected:
		scheduler_t();
	public:
		~scheduler_t();

		scheduler_t(scheduler_t&& right_) = delete;
		scheduler_t& operator = (scheduler_t&& right_) = delete;
		scheduler_t(const scheduler_t&) = delete;
		scheduler_t& operator = (const scheduler_t&) = delete;

		static scheduler_t g_scheduler;
#endif	//DOXYGEN_SKIP_PROPERTY
	};

	struct local_scheduler
	{
		local_scheduler();
		local_scheduler(scheduler_t & sch);
		~local_scheduler();

		local_scheduler(local_scheduler&& right_) = delete;
		local_scheduler& operator = (local_scheduler&& right_) = delete;
		local_scheduler(const local_scheduler&) = delete;
		local_scheduler& operator = (const local_scheduler&) = delete;
#if RESUMEF_ENABLE_MULT_SCHEDULER
	private:
		scheduler_t* _scheduler_ptr;
#endif
	};


#if !RESUMEF_ENABLE_MULT_SCHEDULER
	//获得当前线程下的调度器
	inline scheduler_t* this_scheduler()
	{
		return &scheduler_t::g_scheduler;
	}
#endif
}
