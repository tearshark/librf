
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

	struct scheduler : public std::enable_shared_from_this<scheduler>
	{
	private:
		//typedef spinlock lock_type;
		typedef std::recursive_mutex lock_type;

		mutable spinlock _mtx_ready;
		task_list _ready_task;

		mutable lock_type _mtx_task;
		task_list _task;
		timer_mgr_ptr	_timer;

		RF_API void new_task(task_base * task);
		void cancel_all_task_();

	public:
		RF_API void run_one_batch();
		RF_API void run_until_notask();

		template<class _Ty>
		inline void operator + (_Ty && t_)
		{
			typedef typename std::conditional<
				std::is_callable_v<_Ty>,
				ctx_task_t<_Ty>,
				task_t<_Ty> >::type task_type;
			return new_task(new task_type(std::forward<_Ty>(t_)));
		}

		inline void push_task_internal(task_base * t_)
		{
			return new_task(t_);
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

	protected:
		RF_API scheduler();
	public:
		RF_API ~scheduler();

		scheduler(scheduler && right_) = delete;
		scheduler & operator = (scheduler && right_) = delete;
		scheduler(const scheduler &) = delete;
		scheduler & operator = (const scheduler &) = delete;

		static scheduler g_scheduler;
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
		scheduler * _scheduler_ptr;
#endif
	};
//--------------------------------------------------------------------------------------------------
#if !RESUMEF_ENABLE_MULT_SCHEDULER
	//获得当前线程下的调度器
	inline scheduler * this_scheduler()
	{
		return &scheduler::g_scheduler;
	}
#endif

#if !defined(_DISABLE_RESUMEF_GO_MACRO)
#define go (*::resumef::this_scheduler()) + 
#define GO (*::resumef::this_scheduler()) + [=]()->resumef::future_vt
#endif

//--------------------------------------------------------------------------------------------------
}
