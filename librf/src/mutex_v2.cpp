#include "../librf.h"

RESUMEF_NS
{
	namespace detail
	{
		void state_mutex_base_t::resume()
		{
			coroutine_handle<> handler = _coro;
			if (handler)
			{
				_coro = nullptr;
				_scheduler->del_final(this);
				handler.resume();
			}
		}

		bool state_mutex_base_t::has_handler() const  noexcept
		{
			return (bool)_coro;
		}
		
		state_base_t* state_mutex_base_t::get_parent() const noexcept
		{
			return _root;
		}


		void state_mutex_t::on_cancel() noexcept
		{
			mutex_v2_impl** oldValue = _value.load(std::memory_order_acquire);
			if (oldValue != nullptr && _value.compare_exchange_strong(oldValue, nullptr, std::memory_order_acq_rel))
			{
				*oldValue = nullptr;
				_thandler.stop();

				this->_coro = nullptr;
			}
		}

		bool state_mutex_t::on_notify(mutex_v2_impl* eptr)
		{
			mutex_v2_impl** oldValue = _value.load(std::memory_order_acquire);
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

		bool state_mutex_t::on_timeout()
		{
			mutex_v2_impl** oldValue = _value.load(std::memory_order_acquire);
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


		void state_mutex_all_t::on_cancel() noexcept
		{
			intptr_t oldValue = _counter.load(std::memory_order_acquire);
			if (oldValue >= 0 && _counter.compare_exchange_strong(oldValue, -1, std::memory_order_acq_rel))
			{
				*_value = false;
				_thandler.stop();

				this->_coro = nullptr;
			}
		}

		bool state_mutex_all_t::on_notify(event_v2_impl*)
		{
			intptr_t oldValue = _counter.load(std::memory_order_acquire);
			if (oldValue <= 0) return false;

			oldValue = _counter.fetch_add(-1, std::memory_order_acq_rel);
			if (oldValue == 1)
			{
				*_value = true;
				_thandler.stop();

				assert(this->_scheduler != nullptr);
				if (this->_coro)
					this->_scheduler->add_generator(this);

				return true;
			}

			return oldValue >= 1;
		}

		bool state_mutex_all_t::on_timeout()
		{
			intptr_t oldValue = _counter.load(std::memory_order_acquire);
			if (oldValue >= 0 && _counter.compare_exchange_strong(oldValue, -1, std::memory_order_acq_rel))
			{
				*_value = false;
				_thandler.reset();

				assert(this->_scheduler != nullptr);
				if (this->_coro)
					this->_scheduler->add_generator(this);

				return true;
			}
			return false;
		}


		
		void mutex_v2_impl::lock_until_succeed(void* sch)
		{
			assert(sch != nullptr);

			for (;;)
			{
				if (try_lock(sch))
					break;
				std::this_thread::yield();
			}
		}

		bool mutex_v2_impl::try_lock(void* sch)
		{
			scoped_lock<detail::mutex_v2_impl::lock_type> lock_(_lock);
			return try_lock_lockless(sch);
		}

		bool mutex_v2_impl::try_lock_until(clock_type::time_point tp, void* sch)
		{
			do
			{
				if (try_lock(sch))
					return true;
				std::this_thread::yield();
			} while (clock_type::now() <= tp);
			return false;
		}

		bool mutex_v2_impl::try_lock_lockless(void* sch) noexcept
		{
			void* oldValue = _owner.load(std::memory_order_relaxed);
			if (oldValue == nullptr)
			{
				_owner.store(sch, std::memory_order_relaxed);
				_counter.fetch_add(1, std::memory_order_relaxed);
				return true;
			}
			if (oldValue == sch)
			{
				_counter.fetch_add(1, std::memory_order_relaxed);
				return true;
			}
			return false;
		}

		bool mutex_v2_impl::unlock(void* sch)
		{
			scoped_lock<lock_type> lock_(_lock);

			void* oldValue = _owner.load(std::memory_order_relaxed);
			if (oldValue == sch)
			{
				if (_counter.fetch_sub(1, std::memory_order_relaxed) == 1)
				{
					_owner.store(nullptr, std::memory_order_relaxed);
					while (!_wait_awakes.empty())
					{
						state_mutex_ptr state = _wait_awakes.front();
						_wait_awakes.pop_front();

						if (state->on_notify(this))
						{
							//锁定状态转移到新的state上
							_owner.store(state->get_root(), std::memory_order_relaxed);
							_counter.fetch_add(1, std::memory_order_relaxed);

							break;
						}
					}
				}

				return true;
			}

			return false;
		}

		void mutex_v2_impl::add_wait_list_lockless(state_mutex_t* state)
		{
			assert(state != nullptr);

			_wait_awakes.push_back(state);
		}
	}

	inline namespace mutex_v2
	{
		mutex_t::mutex_t()
			: _mutex(std::make_shared<detail::mutex_v2_impl>())
		{
		}

		mutex_t::mutex_t(std::adopt_lock_t) noexcept
		{
		}

		mutex_t::~mutex_t() noexcept
		{
		}
	}
}
