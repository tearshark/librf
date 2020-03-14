﻿#include "../librf.h"

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

		void state_event_t::on_cancel() noexcept
		{
			bool* oldValue = _value.load(std::memory_order_acquire);
			if (oldValue != nullptr && _value.compare_exchange_strong(oldValue, nullptr, std::memory_order_acq_rel))
			{
				*oldValue = false;
				_thandler.stop();

				this->_coro = nullptr;
			}
		}

		bool state_event_t::on_notify()
		{
			bool* oldValue = _value.load(std::memory_order_acquire);
			if (oldValue != nullptr && _value.compare_exchange_strong(oldValue, nullptr, std::memory_order_acq_rel))
			{
				*oldValue = true;
				_thandler.stop();

				assert(this->_scheduler != nullptr);
				if (this->_coro)
					this->_scheduler->add_generator(this);

				return true;
			}
			return false;
		}

		bool state_event_t::on_timeout()
		{
			bool* oldValue = _value.load(std::memory_order_acquire);
			if (oldValue != nullptr && _value.compare_exchange_strong(oldValue, nullptr, std::memory_order_acq_rel))
			{
				*oldValue = false;
				_thandler.reset();

				assert(this->_scheduler != nullptr);
				if (this->_coro)
					this->_scheduler->add_generator(this);

				return true;
			}
			return false;
		}

		event_v2_impl::event_v2_impl(bool initially) noexcept
			: _counter(initially ? 1 : 0)
		{
		}

		event_v2_impl::~event_v2_impl()
		{
			for (; _wait_awakes.try_pop() != nullptr;);
		}

		void event_v2_impl::signal_all() noexcept
		{
			scoped_lock<lock_type> lock_(_lock);

			_counter.store(0, std::memory_order_release);

			counted_ptr<state_event_t> state;
			for (; (state = _wait_awakes.try_pop()) != nullptr;)
			{
				(void)state->on_notify();
			}
		}

		void event_v2_impl::signal() noexcept
		{
			scoped_lock<lock_type> lock_(_lock);

			counted_ptr<state_event_t> state;
			for (; (state = _wait_awakes.try_pop()) != nullptr;)
			{
				if (state->on_notify())
					return;
			}

			_counter.fetch_add(1, std::memory_order_acq_rel);
		}
	}

	inline namespace event_v2
	{
		event_t::event_t(bool initially)
			:_event(std::make_shared<detail::event_v2_impl>(initially))
		{
		}

		event_t::~event_t()
		{
		}

		event_t::timeout_awaiter event_t::wait_until_(const clock_type::time_point& tp) const noexcept
		{
			return { _event, tp };
		}
	}
}
