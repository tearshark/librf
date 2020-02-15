
#include "rf_task.h"
#include "scheduler.h"
#include <assert.h>

namespace resumef
{
	state_base_t::~state_base_t()
	{
	}

	void state_base_t::set_exception(std::exception_ptr e)
	{
		scoped_lock<lock_type> __guard(this->_mtx);

		this->_exception = std::move(e);
		scheduler_t* sch = this->get_scheduler();
		if (sch != nullptr)
			sch->add_ready(this);
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
		scheduler_t* sch = this->get_scheduler();
		if (sch != nullptr)
			sch->add_ready(this);
	}
}
