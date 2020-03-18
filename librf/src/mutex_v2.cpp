#include "../librf.h"

RESUMEF_NS
{
	namespace detail
	{
		void state_mutex_t::resume()
		{
			coroutine_handle<> handler = _coro;
			if (handler)
			{
				_coro = nullptr;
				_scheduler->del_final(this);
				handler.resume();
			}
		}

		bool state_mutex_t::has_handler() const  noexcept
		{
			return (bool)_coro;
		}
		
		state_base_t* state_mutex_t::get_parent() const noexcept
		{
			return _root;
		}

		bool state_mutex_t::on_notify()
		{
			assert(this->_scheduler != nullptr);
			if (this->_coro)
			{
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

		bool mutex_v2_impl::try_lock(void* sch) noexcept
		{
			scoped_lock<detail::mutex_v2_impl::lock_type> lock_(_lock);
			return try_lock_lockless(sch);
		}

		bool mutex_v2_impl::try_lock_lockless(void* sch) noexcept
		{
			void* oldValue = _owner.load(std::memory_order_relaxed);
			if (oldValue == nullptr || oldValue == sch)
			{
				_owner.store(sch, std::memory_order_relaxed);
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
					if (!_wait_awakes.empty())
					{
						state_mutex_ptr state = _wait_awakes.front();
						_wait_awakes.pop_front();

						//锁定状态转移到新的state上
						_owner.store(state->get_root(), std::memory_order_relaxed);
						_counter.fetch_add(1, std::memory_order_relaxed);

						state->on_notify();
					}
					else
					{
						_owner.store(nullptr, std::memory_order_relaxed);
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
