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
		"timer_canceled",
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
	thread_local scheduler_t * th_scheduler_ptr = nullptr;

	//获得当前线程下的调度器
	scheduler_t * this_scheduler()
	{
		return th_scheduler_ptr ? th_scheduler_ptr : &scheduler_t::g_scheduler;
	}
#endif

	local_scheduler::local_scheduler()
	{
#if RESUMEF_ENABLE_MULT_SCHEDULER
		if (th_scheduler_ptr == nullptr)
		{
			_scheduler_ptr = new scheduler_t;
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

	scheduler_t::scheduler_t()
		: _task()
		, _ready_task()
		, _timer(std::make_shared<timer_manager>())
	{
	}

	scheduler_t::~scheduler_t()
	{
		cancel_all_task_();
#if RESUMEF_ENABLE_MULT_SCHEDULER
		if (th_scheduler_ptr == this)
			th_scheduler_ptr = nullptr;
#endif
	}

	void scheduler_t::new_task(task_base_t * task)
	{
		if (task)
		{
			scoped_lock<spinlock> __guard(_mtx_ready);

			this->_ready_task.push_back(task);
			this->add_initial(task->get_state());
		}
	}

	void scheduler_t::cancel_all_task_()
	{
		{
			scoped_lock<lock_type> __guard(_mtx_task);
			this->_task.clear(true);
		}
		{
			scoped_lock<spinlock> __guard(_mtx_ready);
			this->_ready_task.clear(true);
		}
	}

	void scheduler_t::break_all()
	{
		cancel_all_task_();

		scoped_lock<lock_type> __guard(_mtx_task);
		this->_timer->clear();
	}

	void scheduler_t::run_one_batch()
	{
#if RESUMEF_ENABLE_MULT_SCHEDULER
		if (th_scheduler_ptr == nullptr)
			th_scheduler_ptr = this;
#endif
		{
			scoped_lock<lock_type> __guard(_mtx_task);

			this->_timer->update();

			state_vector states = std::move(_runing_states);
			for (state_sptr& sptr : states)
				sptr->resume();
		}
	}

	void scheduler_t::run_until_notask()
	{
		while (!this->empty())
			this->run_one_batch();
	}

	void scheduler_t::run()
	{
		for (;;)
			this->run_one_batch();
	}

	scheduler_t scheduler_t::g_scheduler;
}
