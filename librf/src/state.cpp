
#include "rf_task.h"
#include "scheduler.h"
#include <assert.h>

namespace resumef
{
	state_base_t::~state_base_t()
	{
	}

	void state_generator_t::resume()
	{
		if (_coro != nullptr)
		{
			_coro.resume();
			if (_coro.done())
			{
				_coro = nullptr;
				_scheduler->del_final(this);
			}
			else
			{
				_scheduler->add_generator(this);
			}
		}
	}

	bool state_generator_t::is_ready() const
	{
		return _coro != nullptr && !_coro.done();
	}

	bool state_generator_t::has_handler() const
	{
		return _coro != nullptr;
	}

	void state_future_t::resume()
	{
		coroutine_handle<> handler;

		scoped_lock<lock_type> __guard(_mtx);
		if (_initor != nullptr)
		{
			handler = _initor;
			_initor = nullptr;
			handler();
		}
		else if (_coro != nullptr)
		{
			handler = _coro;
			_coro = nullptr;
			handler();
		}
	}

	bool state_future_t::has_handler() const
	{
		return _initor != nullptr || _coro != nullptr;
	}

	void state_future_t::set_exception(std::exception_ptr e)
	{
		scoped_lock<lock_type> __guard(this->_mtx);

		this->_exception = std::move(e);
		scheduler_t* sch = this->get_scheduler();
		if (sch != nullptr)
			sch->add_ready(this);
	}
	
	bool state_t<void>::is_ready() const
	{
		scoped_lock<lock_type> __guard(this->_mtx);
		return _is_awaitor == false || _has_value || _exception != nullptr;
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
