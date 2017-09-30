
#include "rf_task.h"
#include "scheduler.h"
#include <assert.h>

namespace resumef
{
	task_base::task_base()
	{
#if RESUMEF_DEBUG_COUNTER
		++g_resumef_task_count;
#endif
	}

	task_base::~task_base()
	{
#if RESUMEF_DEBUG_COUNTER
		--g_resumef_task_count;
#endif
	}

	template<class state_tt>
	struct awaitable_task_t : public task_base
	{
		counted_ptr<state_tt> _state;

		awaitable_task_t() {}
		awaitable_task_t(state_tt * state)
			: _state(state)
		{
		}

		virtual bool is_suspend() override
		{
			return !_state->ready();
		}
		virtual bool go_next(scheduler * schdler) override
		{
			_state->current_scheduler(schdler);
			_state->resume();
			return false;
		}
		virtual void cancel() override
		{
			_state->cancel();
		}
		virtual void * get_id() override
		{
			return _state.get();
		}
	};

	state_base::~state_base()
	{
#if RESUMEF_DEBUG_COUNTER
		--g_resumef_state_count;
#endif
	}

	void state_base::set_value_none_lock()
	{
		// Set all members first as calling coroutine may reset stuff here.
		_ready = true;

		auto sch_ = this->parent_scheduler();
		sch_->push_task_internal(new awaitable_task_t<state_base>(this));
	}

	void state_base::set_exception(std::exception_ptr && e_)
	{
		scoped_lock<std::mutex> __guard(_mtx);

		_exception = std::move(e_);

		// Set all members first as calling coroutine may reset stuff here.
		_ready = true;

		auto sch_ = this->parent_scheduler();
		sch_->push_task_internal(new awaitable_task_t<state_base>(this));
	}
}
