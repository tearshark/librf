
#include "rf_task.h"
#include "scheduler.h"
#include <assert.h>

namespace resumef
{
	std::atomic<intptr_t> g_resumef_static_count = {0};

	state_base_t::~state_base_t()
	{
	}

	void state_base_t::promise_final_suspend()
	{
		scoped_lock<lock_type> __guard(this->_mtx);

		scheduler_t* _scheduler = this->_scheduler;
		if (_scheduler != nullptr)
			_scheduler->del_final(this);
	}

	void state_base_t::set_exception(std::exception_ptr e)
	{
		scoped_lock<lock_type> __guard(this->_mtx);

		this->_exception = std::move(e);
		if (this->_scheduler != nullptr)
			this->_scheduler->add_ready(this);
	}

	void state_t<void>::future_await_resume()
	{
		scoped_lock<lock_type> __guard(this->_mtx);
		if (this->_exception)
			std::rethrow_exception(std::move(this->_exception));
	}

	void state_t<void>::set_value()
	{
		scoped_lock<lock_type> __guard(this->_mtx);

		this->_has_value = true;
		if (this->_scheduler != nullptr)
			this->_scheduler->add_ready(this);
	}
}
