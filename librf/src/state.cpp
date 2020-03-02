#include "../librf.h"

RESUMEF_NS
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
#if RESUMEF_INLINE_STATE
		char* _Ptr = reinterpret_cast<char*>(this) + _Size;
		_Size = *reinterpret_cast<uint32_t*>(_Ptr);
#endif
#if RESUMEF_DEBUG_COUNTER
		std::cout << "destroy_deallocate, size=" << _Size << std::endl;
#endif
		this->~state_generator_t();

		_Alloc_char _Al;
		return _Al.deallocate(reinterpret_cast<char*>(this), _Size);
	}

	void state_generator_t::resume()
	{
		if (_coro)
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

	bool state_generator_t::has_handler() const
	{
		return (bool)_coro;
	}
	
	bool state_generator_t::switch_scheduler_await_suspend(scheduler_t* sch, coroutine_handle<>)
	{
		assert(sch != nullptr);

		if (_scheduler != nullptr)
		{
			auto task_ptr = _scheduler->del_switch(this);
			
			_scheduler = sch;
			if (task_ptr != nullptr)
				sch->add_switch(std::move(task_ptr));
		}
		else
		{
			_scheduler = sch;
		}

		sch->add_generator(this);

		return true;
	}

	void state_future_t::resume()
	{
		std::unique_lock<lock_type> __guard(_mtx);

		if (_is_initor == initor_type::Initial)
		{
			assert((bool)_initor);

			coroutine_handle<> handler = _initor;
			_is_initor = initor_type::None;
			__guard.unlock();

			handler.resume();
			return;
		}
		
		if (_coro)
		{
			coroutine_handle<> handler = _coro;
			_coro = nullptr;
			__guard.unlock();

			handler.resume();
			return;
		}

		if (_is_initor == initor_type::Final)
		{
			assert((bool)_initor);

			coroutine_handle<> handler = _initor;
			_is_initor = initor_type::None;
			__guard.unlock();

			handler.destroy();
			return;
		}
	}

	bool state_future_t::has_handler() const
	{
		scoped_lock<lock_type> __guard(_mtx);
		return has_handler_skip_lock();
	}

	void state_future_t::set_exception(std::exception_ptr e)
	{
		scoped_lock<lock_type> __guard(this->_mtx);
		this->_exception = std::move(e);

		scheduler_t* sch = this->get_scheduler();
		if (sch != nullptr)
		{
			if (this->has_handler_skip_lock())
				sch->add_generator(this);
			else
				sch->del_final(this);
		}
	}

	bool state_future_t::switch_scheduler_await_suspend(scheduler_t* sch, coroutine_handle<> handler)
	{
		assert(sch != nullptr);
		scoped_lock<lock_type> __guard(this->_mtx);

		if (_scheduler != nullptr)
		{
			auto task_ptr = _scheduler->del_switch(this);

			_scheduler = sch;
			if (task_ptr != nullptr)
				sch->add_switch(std::move(task_ptr));
		}
		else
		{
			_scheduler = sch;
		}

		if (_parent != nullptr)
			_parent->switch_scheduler_await_suspend(sch, nullptr);

		if (handler)
		{
			_coro = handler;
			sch->add_generator(this);
		}

		return true;
	}

	void state_t<void>::future_await_resume()
	{
		scoped_lock<lock_type> __guard(this->_mtx);

		if (this->_exception)
			std::rethrow_exception(std::move(this->_exception));
		if (!this->_has_value.load(std::memory_order_acquire))
			std::rethrow_exception(std::make_exception_ptr(future_exception{error_code::not_ready}));
	}

	void state_t<void>::set_value()
	{
		scoped_lock<lock_type> __guard(this->_mtx);
		this->_has_value.store(true, std::memory_order_release);

		scheduler_t* sch = this->get_scheduler();
		if (sch != nullptr)
		{
			if (this->has_handler_skip_lock())
				sch->add_generator(this);
			else
				sch->del_final(this);
		}
	}
}
