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

		void state_event_t::on_cancel() noexcept
		{
			event_v2_impl** oldValue = _value.load(std::memory_order_acquire);
			if (oldValue != nullptr && _value.compare_exchange_strong(oldValue, nullptr, std::memory_order_acq_rel))
			{
				*oldValue = nullptr;
				_thandler.stop();

				this->_coro = nullptr;
			}
		}

		bool state_event_t::on_notify(event_v2_impl* eptr)
		{
			event_v2_impl** oldValue = _value.load(std::memory_order_acquire);
			if (oldValue != nullptr && _value.compare_exchange_strong(oldValue, nullptr, std::memory_order_acq_rel))
			{
				*oldValue = eptr;
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
			event_v2_impl** oldValue = _value.load(std::memory_order_acquire);
			if (oldValue != nullptr && _value.compare_exchange_strong(oldValue, nullptr, std::memory_order_acq_rel))
			{
				*oldValue = nullptr;
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

		template<class _Ty, class _Ptr>
		static auto try_pop_list(intrusive_link_queue<_Ty, _Ptr>& list)
		{
			return list.try_pop();
		}
		template<class _Ptr>
		static _Ptr try_pop_list(std::list<_Ptr>& list)
		{
			if (!list.empty())
			{
				_Ptr ptr = list.front();
				list.pop_front();
				return ptr;
			}
			return nullptr;
		}
		template<class _Ty, class _Ptr>
		static void clear_list(intrusive_link_queue<_Ty, _Ptr>& list)
		{
			for (; list.try_pop() != nullptr;);
		}
		template<class _Ptr>
		static void clear_list(std::list<_Ptr>& list)
		{
			list.clear();
		}

		event_v2_impl::~event_v2_impl()
		{
			clear_list(_wait_awakes);
		}

		void event_v2_impl::signal_all() noexcept
		{
			scoped_lock<lock_type> lock_(_lock);

			_counter.store(0, std::memory_order_release);

			state_event_ptr state;
			for (; (state = try_pop_list(_wait_awakes)) != nullptr;)
			{
				(void)state->on_notify(this);
			}
		}

		void event_v2_impl::signal() noexcept
		{
			scoped_lock<lock_type> lock_(_lock);

			state_event_ptr state;
			for (; (state = try_pop_list(_wait_awakes)) != nullptr;)
			{
				if (state->on_notify(this))
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

		event_t::event_t(std::adopt_lock_t)
		{
		}

		event_t::~event_t()
		{
		}

		event_t::timeout_awaiter event_t::wait_until_(const clock_type::time_point& tp) const noexcept
		{
			return { _event.get(), tp };
		}
	}
}
