
#pragma once

#include <array>
#include <vector>
//#include <yvals.h>

#include "rf_task.h"
#include "task_list.h"
#include "utils.h"
#include "timer.h"

namespace resumef
{
	struct local_scheduler;

	struct scheduler_t : public std::enable_shared_from_this<scheduler_t>
	{
		using state_sptr = std::shared_ptr<state_base_t>;
		using state_vector = std::vector<state_sptr>;
	private:
		state_vector	_runing_states;
	private:
		//typedef spinlock lock_type;
		typedef std::recursive_mutex lock_type;

		mutable spinlock _mtx_ready;
		task_list _ready_task;

		mutable lock_type _mtx_task;
		task_list _task;
		timer_mgr_ptr	_timer;

		RF_API void new_task(task_base_t * task);
		void cancel_all_task_();

	public:
		RF_API void run_one_batch();
		RF_API void run_until_notask();
		RF_API void run();

		template<class _Ty, typename = std::enable_if_t<std::is_callable_v<_Ty> || is_future_v<_Ty>>>
		inline void operator + (_Ty && t_)
		{
			if constexpr(is_future_v<_Ty>)
				new_task(new task_t<_Ty>(std::forward<_Ty>(t_)));
			else
				new_task(new ctx_task_t<_Ty>(std::forward<_Ty>(t_)));
		}

		inline void push_task_internal(task_base_t * t_)
		{
			new_task(t_);
		}

		inline bool empty() const
		{
			scoped_lock<spinlock, lock_type> __guard(_mtx_ready, _mtx_task);

			return this->_task.empty() && this->_ready_task.empty() && this->_timer->empty();
		}

		RF_API void break_all();

		inline timer_manager * timer() const
		{
			return _timer.get();
		}

		friend struct task_base;
		friend struct local_scheduler;

		void add_initial(state_base_t* sptr)
		{
			sptr->_scheduler = this;
			assert(sptr->_coro != nullptr);
			_runing_states.emplace_back(sptr);
		}

		void add_await(state_base_t* sptr, coroutine_handle<> handler)
		{
			sptr->_scheduler = this;
			sptr->_coro = handler;
			if (sptr->has_value() || sptr->_exception != nullptr)
				_runing_states.emplace_back(sptr);
		}

		void add_ready(state_base_t* sptr)
		{
			assert(sptr->_scheduler == this);
			if (sptr->_coro != nullptr)
				_runing_states.emplace_back(sptr);
		}
	protected:
		RF_API scheduler_t();
	public:
		RF_API ~scheduler_t();

		scheduler_t(scheduler_t&& right_) = delete;
		scheduler_t& operator = (scheduler_t&& right_) = delete;
		scheduler_t(const scheduler_t&) = delete;
		scheduler_t& operator = (const scheduler_t&) = delete;

		static scheduler_t g_scheduler;
	};

	struct local_scheduler
	{
		RF_API local_scheduler();
		RF_API ~local_scheduler();

		local_scheduler(local_scheduler && right_) = delete;
		local_scheduler & operator = (local_scheduler && right_) = delete;
		local_scheduler(const local_scheduler &) = delete;
		local_scheduler & operator = (const local_scheduler &) = delete;
#if RESUMEF_ENABLE_MULT_SCHEDULER
	private:
		scheduler_t* _scheduler_ptr;
#endif
	};
//--------------------------------------------------------------------------------------------------
#if !RESUMEF_ENABLE_MULT_SCHEDULER
	//获得当前线程下的调度器
	inline scheduler_t* this_scheduler()
	{
		return &scheduler_t::g_scheduler;
	}
#endif

#if !defined(_DISABLE_RESUMEF_GO_MACRO)
#define go (*::resumef::this_scheduler()) + 
#define GO (*::resumef::this_scheduler()) + [=]()mutable->resumef::future_t<>
#endif

//--------------------------------------------------------------------------------------------------
}
