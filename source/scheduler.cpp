#include "librf/librf.h"

#if RESUMEF_DEBUG_COUNTER
std::mutex g_resumef_cout_mutex;
std::atomic<intptr_t> g_resumef_state_count = 0;
std::atomic<intptr_t> g_resumef_task_count = 0;
std::atomic<intptr_t> g_resumef_evtctx_count = 0;
std::atomic<intptr_t> g_resumef_state_id = 0;
#endif

namespace librf
{
	const char * future_error_string[(size_t)error_code::max__]
	{
		"none",
		"not_ready",
		"already_acquired",
		"unlock_more",
		"read_before_write",
		"timer_canceled",
		"not_await_lock",
		"stop_requested",
	};

	char sz_future_error_buffer[256];

	LIBRF_API const char * get_error_string(error_code fe, const char * classname)
	{
		if (classname)
		{
#if defined(__clang__) || defined(__GNUC__)
#define sprintf_s sprintf
#endif
			sprintf_s(sz_future_error_buffer, "%s, code=%s", classname, future_error_string[(size_t)(fe)]);
			return sz_future_error_buffer;
		}
		return future_error_string[(size_t)(fe)];
	}

	thread_local scheduler_t * th_scheduler_ptr = nullptr;

	//获得当前线程下的调度器
	LIBRF_API scheduler_t * this_scheduler()
	{
		return th_scheduler_ptr ? th_scheduler_ptr : &scheduler_t::g_scheduler;
	}

	LIBRF_API local_scheduler_t::local_scheduler_t()
	{
		if (th_scheduler_ptr == nullptr)
		{
			_scheduler_ptr = new scheduler_t;
			th_scheduler_ptr = _scheduler_ptr;
		}
		else
		{
			_scheduler_ptr = nullptr;
		}
	}

	LIBRF_API local_scheduler_t::local_scheduler_t(scheduler_t& sch) noexcept
	{
		if (th_scheduler_ptr == nullptr)
		{
			th_scheduler_ptr = &sch;
		}

		_scheduler_ptr = nullptr;
	}

	LIBRF_API local_scheduler_t::~local_scheduler_t()
	{
		if (th_scheduler_ptr == _scheduler_ptr)
			th_scheduler_ptr = nullptr;
		delete _scheduler_ptr;
	}

	LIBRF_API scheduler_t::scheduler_t()
		: _timer(std::make_shared<timer_manager>())
	{
		_runing_states.reserve(1024);
		_cached_states.reserve(1024);

		if (th_scheduler_ptr == nullptr)
			th_scheduler_ptr = this;
	}

	LIBRF_API scheduler_t::~scheduler_t()
	{
		//cancel_all_task_();
		if (th_scheduler_ptr == this)
			th_scheduler_ptr = nullptr;
	}

	LIBRF_API task_t* scheduler_t::new_task(task_t * task)
	{
		state_base_t* sptr = task->_state.get();
		sptr->set_scheduler(this);

		{
#if !RESUMEF_DISABLE_MULT_THREAD
			scoped_lock<spinlock> __guard(_lock_ready);
#endif
			_ready_task.emplace(sptr, task);
		}

		//如果是单独的future，没有被co_await过，则handler是nullptr。
		if (sptr->has_handler())
		{
			add_generator(sptr);
		}

		return task;
	}

	LIBRF_API std::unique_ptr<task_t> scheduler_t::del_switch(state_base_t* sptr)
	{
#if !RESUMEF_DISABLE_MULT_THREAD
		scoped_lock<spinlock> __guard(_lock_ready);
#endif
	
		std::unique_ptr<task_t> task_ptr;

		auto iter = this->_ready_task.find(sptr);
		if (iter != this->_ready_task.end())
		{
			task_ptr = std::exchange(iter->second, nullptr);
			this->_ready_task.erase(iter);
		}

		return task_ptr;
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

	LIBRF_API bool scheduler_t::run_one_batch()
	{
		this->_timer->update();

		{
#if !RESUMEF_DISABLE_MULT_THREAD
			scoped_lock<spinlock> __guard(_lock_running);
#endif
			if (likely(_runing_states.empty()))
				return false;

			std::swap(_cached_states, _runing_states);
		}

		for (state_sptr& sptr : _cached_states)
			sptr->resume();

		_cached_states.clear();
		return true;
	}

	LIBRF_API void scheduler_t::run_until_notask()
	{
		for(;;)
		{
			//介于网上有人做评测，导致单协程切换数据很难看，那就注释掉吧。
			//std::this_thread::yield();

			if (likely(this->run_one_batch())) continue;	//当前运行了一个state，则认为还可能有任务未完成

			{
#if !RESUMEF_DISABLE_MULT_THREAD
				scoped_lock<spinlock> __guard(_lock_ready);
#endif
				if (likely(!_ready_task.empty())) continue;	//当前还存在task，则必然还有任务未完成
			}
			if (unlikely(!_timer->empty())) continue;			//定时器不为空，也需要等待定时器触发

			break;
		};
	}

	LIBRF_API scheduler_t scheduler_t::g_scheduler;
}
