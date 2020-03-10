#include "../librf.h"

RESUMEF_NS
{
	namespace detail
	{
		void state_event_t::resume()
		{
			coroutine_handle<> handler = _coro;
			if (handler)
			{
				_coro = nullptr;
				_scheduler->del_final(this);
				handler.resume();
			}
		}

		bool state_event_t::has_handler() const  noexcept
		{
			return (bool)_coro;
		}

		void state_event_t::on_notify()
		{
			*_value = true;

			assert(this->_scheduler != nullptr);
			if (this->_coro)
				this->_scheduler->add_generator(this);
		}

		void state_event_t::on_cancel() noexcept
		{
			*_value = false;
			this->_coro = nullptr;
		}

		void event_v2_impl::signal_all() noexcept
		{
			scoped_lock<lock_type> lock_(_lock);

			_counter.store(0, std::memory_order_release);

			state_event_t* state;
			for (; (state = _wait_awakes.try_pop()) != nullptr;)
			{
				state->on_notify();
			}
		}

		void event_v2_impl::signal() noexcept
		{
			scoped_lock<lock_type> lock_(_lock);

			state_event_t* state = _wait_awakes.try_pop();
			if (state == nullptr)
				_counter.fetch_add(1, std::memory_order_acq_rel);
			else
				state->on_notify();
		}
	}

	namespace event_v2
	{
		event_t::event_t(bool initially)
			:_event(std::make_shared<detail::event_v2_impl>(initially))
		{
		}

		event_t::~event_t()
		{
		}
	}
}
