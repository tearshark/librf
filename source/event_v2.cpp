#include "librf/librf.h"

namespace librf
{
	namespace detail
	{

		LIBRF_API void state_event_t::on_cancel() noexcept
		{
			event_v2_impl** oldValue = _value.load(std::memory_order_acquire);
			if (oldValue != nullptr && _value.compare_exchange_strong(oldValue, nullptr, std::memory_order_acq_rel))
			{
				*oldValue = nullptr;
				_thandler.stop();

				this->_coro = nullptr;
			}
		}

		LIBRF_API bool state_event_t::on_notify(event_v2_impl* eptr)
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

		LIBRF_API bool state_event_t::on_timeout()
		{
			event_v2_impl** oldValue = _value.load(std::memory_order_acquire);
			if (oldValue != nullptr && _value.compare_exchange_strong(oldValue, nullptr, std::memory_order_acq_rel))
			{
				event_v2_impl* evt = *oldValue;
				if (evt != nullptr)
					evt->remove_wait_list(this);
				*oldValue = nullptr;
				_thandler.reset();

				assert(this->_scheduler != nullptr);
				if (this->_coro)
					this->_scheduler->add_generator(this);

				return true;
			}
			return false;
		}




		LIBRF_API void state_event_all_t::on_cancel(intptr_t idx)
		{
			scoped_lock<event_v2_impl::lock_type> lock_(_lock);
			
			if (_counter <= 0) return ;
			assert(idx < static_cast<intptr_t>(_values.size()));

			_values[idx] = sub_state_t{ nullptr, nullptr };
			if (--_counter == 0)
			{
				*_result = false;
				_thandler.stop();

				assert(this->_scheduler != nullptr);
				if (this->_coro)
					this->_scheduler->add_generator(this);
			}
		}

		LIBRF_API bool state_event_all_t::on_notify(event_v2_impl*, intptr_t idx)
		{
			scoped_lock<event_v2_impl::lock_type> lock_(_lock);

			if (_counter <= 0) return false;
			assert(idx < static_cast<intptr_t>(_values.size()));

			_values[idx].first = nullptr;

			if (--_counter == 0)
			{
				bool result = true;
				for (sub_state_t& sub : _values)
				{
					if (sub.second == nullptr)
					{
						result = false;
						break;
					}
				}

				*_result = result;
				_thandler.stop();

				assert(this->_scheduler != nullptr);
				if (this->_coro)
					this->_scheduler->add_generator(this);
			}

			return true;
		}

		LIBRF_API bool state_event_all_t::on_timeout()
		{
			scoped_lock<event_v2_impl::lock_type> lock_(_lock);

			if (_counter <= 0) return false;

			_counter = 0;
			*_result = false;
			_thandler.reset();

			for (sub_state_t& sub : _values)
			{
				if (sub.first != nullptr)
				{
					event_v2_impl* evt = sub.second;
					sub.second = nullptr;

					if (evt != nullptr)
						evt->remove_wait_list(sub.first);
				}
			}

			assert(this->_scheduler != nullptr);
			if (this->_coro)
				this->_scheduler->add_generator(this);

			return true;
		}



		LIBRF_API event_v2_impl::event_v2_impl(bool initially) noexcept
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

		LIBRF_API event_v2_impl::~event_v2_impl()
		{
			clear_list(_wait_awakes);
		}

		LIBRF_API void event_v2_impl::signal_all() noexcept
		{
			scoped_lock<lock_type> lock_(_lock);

			state_event_ptr state;
			for (; (state = try_pop_list(_wait_awakes)) != nullptr;)
			{
				(void)state->on_notify(this);
			}
		}

		LIBRF_API void event_v2_impl::signal() noexcept
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

		LIBRF_API void event_v2_impl::reset() noexcept
		{
			_counter.store(0, std::memory_order_release);
		}

		LIBRF_API void event_v2_impl::add_wait_list(state_event_base_t* state)
		{
			assert(state != nullptr);
			_wait_awakes.push_back(state);
		}

		LIBRF_API void event_v2_impl::remove_wait_list(state_event_base_t* state)
		{
			assert(state != nullptr);

			scoped_lock<lock_type> lock_(_lock);
			_wait_awakes.erase(state);
		}
	}

	LIBRF_API event_t::event_t(bool initially)
		:_event(std::make_shared<detail::event_v2_impl>(initially))
	{
	}

	LIBRF_API event_t::event_t(std::adopt_lock_t)
	{
	}

	LIBRF_API event_t::~event_t()
	{
	}
}
