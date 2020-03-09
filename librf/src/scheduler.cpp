#include "../librf.h"

#if RESUMEF_DEBUG_COUNTER
std::mutex g_resumef_cout_mutex;
std::atomic<intptr_t> g_resumef_state_count = 0;
std::atomic<intptr_t> g_resumef_task_count = 0;
std::atomic<intptr_t> g_resumef_evtctx_count = 0;
std::atomic<intptr_t> g_resumef_state_id = 0;
#endif

RESUMEF_NS
{
	const char * future_error_string[(size_t)error_code::max__]
	{
		"none",
		"not_ready",
		"already_acquired",
		"unlock_more",
		"read_before_write",
		"timer_canceled",
	};

	char sz_future_error_buffer[256];

	const char * get_error_string(error_code fe, const char * classname)
	{
		if (classname)
		{
#if __clang__
#define sprintf_s sprintf
#endif
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
		else
		{
			_scheduler_ptr = nullptr;
		}
#endif
	}

	local_scheduler::local_scheduler(scheduler_t& sch)
	{
#if RESUMEF_ENABLE_MULT_SCHEDULER
		if (th_scheduler_ptr == nullptr)
		{
			th_scheduler_ptr = &sch;
		}

		_scheduler_ptr = nullptr;
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
		: _timer(std::make_shared<timer_manager>())
	{
		_runing_states.reserve(1024);
		_cached_states.reserve(1024);

#if RESUMEF_ENABLE_MULT_SCHEDULER
		if (th_scheduler_ptr == nullptr)
			th_scheduler_ptr = this;
#endif
	}

	scheduler_t::~scheduler_t()
	{
		//cancel_all_task_();
#if RESUMEF_ENABLE_MULT_SCHEDULER
		if (th_scheduler_ptr == this)
			th_scheduler_ptr = nullptr;
#endif
	}

	void scheduler_t::new_task(task_base_t * task)
	{
		state_base_t* sptr = task->get_state();
		sptr->set_scheduler(this);

		{
			scoped_lock<spinlock, spinlock> __guard(_lock_ready, _lock_running);
			_ready_task.emplace(sptr, task);
		}

		//如果是单独的future，没有被co_await过，则handler是nullptr。
		if (sptr->has_handler())
		{
			add_generator(sptr);
		}
	}

	void scheduler_t::add_generator(state_base_t* sptr)
	{
		assert(sptr != nullptr);

		scoped_lock<spinlock> __guard(_lock_running);
		_runing_states.emplace_back(sptr);
	}

	void scheduler_t::del_final(state_base_t* sptr)
	{
		scoped_lock<spinlock> __guard(_lock_ready);
		this->_ready_task.erase(sptr);
	}

	std::unique_ptr<task_base_t> scheduler_t::del_switch(state_base_t* sptr)
	{
		scoped_lock<spinlock> __guard(_lock_ready);
	
		std::unique_ptr<task_base_t> task_ptr;

		auto iter = this->_ready_task.find(sptr);
		if (iter != this->_ready_task.end())
		{
			task_ptr = std::move(iter->second);
			this->_ready_task.erase(iter);
		}

		return task_ptr;
	}

	void scheduler_t::add_switch(std::unique_ptr<task_base_t> task)
	{
		state_base_t* sptr = task->get_state();

		scoped_lock<spinlock> __guard(_lock_ready);
		this->_ready_task.emplace(sptr, std::move(task));
	}

/*
	void scheduler_t::cancel_all_task_()
	{
		scoped_lock<spinlock, spinlock> __guard(_lock_ready, _lock_running);
		
		this->_ready_task.clear();
		this->_runing_states.clear();
	}

	void scheduler_t::break_all()
	{
		cancel_all_task_();
		this->_timer->clear();
	}
*/

	void scheduler_t::run_one_batch()
	{
		this->_timer->update();

		{
			scoped_lock<spinlock> __guard(_lock_running);
			if (_runing_states.empty())
				return;

			std::swap(_cached_states, _runing_states);
		}

		for (state_sptr& sptr : _cached_states)
			sptr->resume();

		_cached_states.clear();
	}

	void scheduler_t::run_until_notask()
	{
		while (!this->empty())
		{
			this->run_one_batch();
			std::this_thread::yield();
		}
	}

	void scheduler_t::run()
	{
		for (;;)
		{
			this->run_one_batch();
			std::this_thread::yield();
		}
	}

	scheduler_t scheduler_t::g_scheduler;
}
