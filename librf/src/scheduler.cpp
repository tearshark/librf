#include "scheduler.h"
#include <assert.h>

#if RESUMEF_DEBUG_COUNTER
std::atomic<intptr_t> g_resumef_state_count = 0;
std::atomic<intptr_t> g_resumef_task_count = 0;
std::atomic<intptr_t> g_resumef_evtctx_count = 0;
#endif

namespace resumef
{
	static const char * future_error_string[(size_t)future_error::max__]
	{
		"none",
		"not_ready",
		"already_acquired",
		"unlock_more",
		"read_before_write",
	};

	static char sz_future_error_buffer[256];

	const char * get_error_string(future_error fe, const char * classname)
	{
		if (classname)
		{
			sprintf_s(sz_future_error_buffer, "%s, code=%s", classname, future_error_string[(size_t)(fe)]);
			return sz_future_error_buffer;
		}
		return future_error_string[(size_t)(fe)];
	}

	//获得当前线程下的调度器
	scheduler * this_scheduler()
	{
		return &g_scheduler;
	}

	//获得当前线程下，正在由调度器调度的协程
/*
	namespace detail
	{
		state_base * current_coroutine()
		{
			scheduler * schdler = this_scheduler();
			if (schdler->current_state)
				return schdler->current_state;
			return schdler->top_state();
		}
	}
*/

	scheduler::scheduler()
		: _task()
		, _ready_task()
		, _timer(std::make_shared<timer_manager>())
	{
	}

	scheduler::~scheduler()
	{
		cancel_all_task_();
	}

	scheduler::scheduler(scheduler && right_)
	{
		this->swap(right_);
	}

	scheduler & scheduler::operator = (scheduler && right_)
	{
		this->swap(right_);
		return *this;
	}

	void scheduler::new_task(task_base * task)
	{
		if (task)
		{
			scoped_lock<std::recursive_mutex> __guard(_mtx_ready);
			this->_ready_task.push_back(task);
		}
	}

	void scheduler::cancel_all_task_()
	{
		{
			scoped_lock<std::recursive_mutex> __guard(_mtx_ready);
			for (auto task : this->_ready_task)
			{
				task->cancel();
				delete task;
			}
			this->_ready_task.clear();
		}
		{
			scoped_lock<std::recursive_mutex> __guard(_mtx_task);
			for (auto task : this->_task)
			{
				task->cancel();
				delete task;
			}
			this->_task.clear();
		}
	}

	void scheduler::break_all()
	{
		cancel_all_task_();

		scoped_lock<std::recursive_mutex> __guard(_mtx_task);
		this->_timer->clear();
	}

	void scheduler::run_one_batch()
	{
		{
			scoped_lock<std::recursive_mutex> __guard(_mtx_task);

			this->_timer->update();

			using namespace std::chrono;

			for (auto iter = this->_task.begin(); iter != this->_task.end(); )
			{
#if _DEBUG
#define MAX_TIME_COST 10000us
#else
#define MAX_TIME_COST 1000us
#endif
//				time_cost_evaluation<microseconds> eva(MAX_TIME_COST);

				auto task = *iter;
				if (task->is_suspend() || task->go_next(this))
				{
//					eva.add("coscheduler");
					++iter;
					continue;
				}

				iter = this->_task.erase(iter);
				delete task;
			}
		}
		{
			scoped_lock<std::recursive_mutex> __guard(_mtx_ready);
			if (this->_ready_task.size() > 0)
			{
				this->_task.insert(this->_task.end(), this->_ready_task.begin(), this->_ready_task.end());
				this->_ready_task.clear();
			}
		}
	}

	void scheduler::run_until_notask()
	{
		while (!this->empty())
			this->run_one_batch();
	}

	void scheduler::swap(scheduler & right_)
	{
		if (this != &right_)
		{
			scoped_lock<std::recursive_mutex, std::recursive_mutex, std::recursive_mutex, std::recursive_mutex>
				__guard(this->_mtx_ready, this->_mtx_task, right_._mtx_ready, right_._mtx_task);

			std::swap(this->_ready_task, right_._ready_task);
			std::swap(this->_task, right_._task);
			std::swap(this->_timer, right_._timer);
		}
	}

	scheduler g_scheduler;
}
