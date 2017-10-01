
#include "rf_task.h"
#include "scheduler.h"
#include <assert.h>

namespace resumef
{
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
		virtual void bind(scheduler * ) override
		{
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

		if (_coro)
		{
			auto sch_ = this->current_scheduler();
/*
			if (sch_ == nullptr)
				sch_ = this_scheduler();
*/
			sch_->push_task_internal(new awaitable_task_t<state_base>(this));
		}
	}

	void state_base::set_exception(std::exception_ptr && e_)
	{
		scoped_lock<std::mutex> __guard(_mtx);

		_exception = std::move(e_);
		// Set all members first as calling coroutine may reset stuff here.
		_ready = true;

		if (_coro)
		{
			auto sch_ = this->current_scheduler();
/*
			if (sch_ == nullptr)
				sch_ = this_scheduler();
*/
			sch_->push_task_internal(new awaitable_task_t<state_base>(this));
		}
	}

	void state_base::await_suspend(coroutine_handle<> resume_cb)
	{
		_coro = resume_cb;

		if (_current_scheduler == nullptr)
		{
			auto * promise_ = this->parent_promise();
			if (promise_)
			{
				scheduler * sch_ = promise_->_state->current_scheduler();
				if (sch_)
					this->current_scheduler(sch_);
				else
					promise_->_state->_depend_states.push_back(counted_ptr<state_base>(this));
			}
		}
		else
		{
			for (auto & stptr : _depend_states)
				stptr->current_scheduler(_current_scheduler);
			_depend_states.clear();
		}
	}

	void state_base::current_scheduler(scheduler * sch_)
	{
		_current_scheduler = sch_;

		for (auto & stptr : _depend_states)
			stptr->current_scheduler(sch_);
		_depend_states.clear();
	}
}
