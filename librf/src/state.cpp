
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
		scoped_lock<lock_type> __guard(_mtx);

		if (_initor != nullptr)
		{
			coroutine_handle<> handler = _initor;
			_initor = nullptr;
			handler.resume();
		}
		else if (_coro != nullptr)
		{
			coroutine_handle<> handler = _coro;
			_coro = nullptr;
			handler.resume();
		}
	}

	bool state_future_t::has_handler() const
	{
		scoped_lock<lock_type> __guard(_mtx);
		return _coro != nullptr || _initor != nullptr;
	}

	bool state_future_t::is_ready() const
	{
		scoped_lock<lock_type> __guard(this->_mtx);
		return _exception != nullptr || _has_value || !_is_awaitor;
	}

	void state_future_t::set_exception(std::exception_ptr e)
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
		if (!this->_has_value)
			std::rethrow_exception(std::make_exception_ptr(future_exception{error_code::not_ready}));
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
