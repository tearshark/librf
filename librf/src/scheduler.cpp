#include "scheduler.h"
#include <assert.h>

#if RESUMEF_DEBUG_COUNTER
std::mutex g_resumef_cout_mutex;
std::atomic<intptr_t> g_resumef_state_count = 0;
std::atomic<intptr_t> g_resumef_task_count = 0;
std::atomic<intptr_t> g_resumef_evtctx_count = 0;
#endif

namespace resumef
{

	static const char * future_error_string[(size_t)error_code::max__]
	{
		"none",
		"not_ready",
		"already_acquired",
		"unlock_more",
		"read_before_write",
	};

	static char sz_future_error_buffer[256];

	const char * get_error_string(error_code fe, const char * classname)
	{
		if (classname)
		{
			sprintf_s(sz_future_error_buffer, "%s, code=%s", classname, future_error_string[(size_t)(fe)]);
			return sz_future_error_buffer;
		}
		return future_error_string[(size_t)(fe)];
	}

#if RESUMEF_ENABLE_MULT_SCHEDULER
	thread_local scheduler * th_scheduler_ptr = nullptr;

	//获得当前线程下的调度器
	scheduler * this_scheduler()
	{
		return th_scheduler_ptr ? th_scheduler_ptr : &scheduler::g_scheduler;
	}
#endif

	local_scheduler::local_scheduler()
	{
#if RESUMEF_ENABLE_MULT_SCHEDULER
		if (th_scheduler_ptr == nullptr)
		{
			_scheduler_ptr = new scheduler;
			th_scheduler_ptr = _scheduler_ptr;
		}
#endif
	}

	local_scheduler::~local_scheduler()
	{
#if RESUMEF_ENABLE_MULT_SCHEDULER
		if (th_scheduler_ptr == _scheduler_ptr)
			th_scheduler_ptr = nullptr;
		delete _scheduler_ptr;
#endif
	}

	scheduler::scheduler()
		: _task()
		, _ready_task()
		, _timer(std::make_shared<timer_manager>())
	{
	}

	scheduler::~scheduler()
	{
		cancel_all_task_();
#if RESUMEF_ENABLE_MULT_SCHEDULER
		if (th_scheduler_ptr == this)
			th_scheduler_ptr = nullptr;
#endif
	}

	void scheduler::new_task(task_base * task)
	{
		if (task)
		{
			scoped_lock<std::recursive_mutex> __guard(_mtx_ready);
#if RESUMEF_ENABLE_MULT_SCHEDULER
			task->bind(this);
#endif
			this->_ready_task.push_back(task);
		}
	}

	void scheduler::cancel_all_task_()
	{
		{
			scoped_lock<std::recursive_mutex> __guard(_mtx_task);
			for (auto task : this->_task)
			{
				task->cancel();
				delete task;
			}
			this->_task.clear();
		}
		{
			scoped_lock<std::recursive_mutex> __guard(_mtx_ready);
			for (auto task : this->_ready_task)
			{
				task->cancel();
				delete task;
			}
			this->_ready_task.clear();
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
#if RESUMEF_ENABLE_MULT_SCHEDULER
		if (th_scheduler_ptr == nullptr)
			th_scheduler_ptr = this;
#endif
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

			{
				scoped_lock<std::recursive_mutex> __guard(_mtx_ready);
				if (this->_ready_task.size() > 0)
				{
					this->_task.insert(this->_task.end(), this->_ready_task.begin(), this->_ready_task.end());
					this->_ready_task.clear();
				}
			}
		}
	}

	void scheduler::run_until_notask()
	{
		while (!this->empty())
			this->run_one_batch();
	}

	scheduler scheduler::g_scheduler;
}
