#include "librf/librf.h"

namespace librf
{
	namespace detail
	{
		LIBRF_API void state_mutex_t::resume()
		{
			coroutine_handle<> handler = _coro;
			if (handler)
			{
				_coro = nullptr;
				_scheduler->del_final(this);
				handler.resume();
			}
		}

		LIBRF_API bool state_mutex_t::has_handler() const  noexcept
		{
			return (bool)_coro;
		}
		
		LIBRF_API state_base_t* state_mutex_t::get_parent() const noexcept
		{
			return _root;
		}


		LIBRF_API void state_mutex_t::on_cancel() noexcept
		{
			mutex_v2_impl** oldValue = _value.load(std::memory_order_acquire);
			if (oldValue != nullptr && _value.compare_exchange_strong(oldValue, nullptr, std::memory_order_acq_rel))
			{
				*oldValue = nullptr;
				_thandler.stop();

				this->_coro = nullptr;
			}
		}

		LIBRF_API bool state_mutex_t::on_notify(mutex_v2_impl* eptr)
		{
			assert(eptr != nullptr);

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

		LIBRF_API bool state_mutex_t::on_timeout()
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

		LIBRF_API void state_mutex_t::add_timeout_timer(std::chrono::system_clock::time_point tp)
		{
			this->_thandler = this->_scheduler->timer()->add_handler(tp,
				[st = counted_ptr<state_mutex_t>{ this }](bool canceld)
				{
					if (!canceld)
						st->on_timeout();
				});
		}



		LIBRF_API void mutex_v2_impl::lock_until_succeed(void* sch)
		{
			assert(sch != nullptr);

			for (;;)
			{
				if (try_lock(sch))
					break;
				std::this_thread::yield();
			}
		}

		LIBRF_API bool mutex_v2_impl::try_lock(void* sch)
		{
			assert(sch != nullptr);

			scoped_lock<detail::mutex_v2_impl::lock_type> lock_(_lock);
			return try_lock_lockless(sch);
		}

		LIBRF_API bool mutex_v2_impl::try_lock_until(clock_type::time_point tp, void* sch)
		{
			assert(sch != nullptr);

			do
			{
				if (try_lock(sch))
					return true;
				std::this_thread::yield();
			} while (clock_type::now() <= tp);
			return false;
		}

		LIBRF_API bool mutex_v2_impl::try_lock_lockless(void* sch) noexcept
		{
			assert(sch != nullptr);

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

		LIBRF_API bool mutex_v2_impl::unlock(void* sch)
		{
			assert(sch != nullptr);

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

						//先将锁定状态转移到新的state上
						_owner.store(state->get_root(), std::memory_order_release);
						_counter.fetch_add(1, std::memory_order_acq_rel);
						
						if (state->on_notify(this))
							break;

						//转移状态失败，恢复成空
						_owner.store(nullptr, std::memory_order_relaxed);
						_counter.fetch_sub(1, std::memory_order_relaxed);
					}
				}

				return true;
			}

			return false;
		}

		LIBRF_API void mutex_v2_impl::add_wait_list_lockless(state_mutex_t* state)
		{
			assert(state != nullptr);

			_wait_awakes.push_back(state);
		}
	}

	LIBRF_API mutex_t::mutex_t()
		: _mutex(std::make_shared<detail::mutex_v2_impl>())
	{
	}

	LIBRF_API mutex_t::mutex_t(std::adopt_lock_t) noexcept
	{
	}

	LIBRF_API mutex_t::~mutex_t() noexcept
	{
	}
}
