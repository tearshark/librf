
#include "rf_task.h"
#include "scheduler.h"
#include <assert.h>

namespace resumef
{
	state_base_t::~state_base_t()
	{
	}

	void state_future_t::destroy_deallocate()
	{
		size_t _Size = this->_alloc_size;
#if RESUMEF_DEBUG_COUNTER
		std::cout << "destroy_deallocate, size=" << _Size << std::endl;
#endif
		this->~state_future_t();

		_Alloc_char _Al;
		return _Al.deallocate(reinterpret_cast<char*>(this), _Size);
	}
	
	void state_generator_t::destroy_deallocate()
	{
		size_t _Size = _Align_size<state_generator_t>();
		char* _Ptr = reinterpret_cast<char*>(this) + _Size;
		_Size = *reinterpret_cast<uint32_t*>(_Ptr);
#if RESUMEF_DEBUG_COUNTER
		std::cout << "destroy_deallocate, size=" << _Size << std::endl;
#endif
		this->~state_generator_t();

		_Alloc_char _Al;
		return _Al.deallocate(reinterpret_cast<char*>(this), _Size);
	}

	void state_generator_t::resume()
	{
		if (_coro != nullptr)
		{
			_coro.resume();
			if (_coro.done())
			{
				coroutine_handle<> handler = _coro;
				_coro = nullptr;
				_scheduler->del_final(this);

				handler.destroy();
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
		std::unique_lock<lock_type> __guard(_mtx);

		if (_initor != nullptr && _is_initor)
		{
			coroutine_handle<> handler = _initor;
			_initor = nullptr;
			__guard.unlock();

			handler.resume();
			return;
		}
		
		if (_coro != nullptr)
		{
			coroutine_handle<> handler = _coro;
			_coro = nullptr;
			__guard.unlock();

			handler.resume();
			return;
		}

		if (_initor != nullptr && !_is_initor)
		{
			coroutine_handle<> handler = _initor;
			_initor = nullptr;
			__guard.unlock();

			handler.destroy();
			return;
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
