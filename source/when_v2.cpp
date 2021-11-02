#include "librf/librf.h"

namespace librf
{
	namespace detail
	{
		LIBRF_API state_when_t::state_when_t(intptr_t counter_)
			:_counter(counter_)
		{
		}

		LIBRF_API void state_when_t::resume()
		{
			coroutine_handle<> handler = _coro;
			if (handler)
			{
				_coro = nullptr;
				_scheduler->del_final(this);
				handler.resume();
			}
		}

		LIBRF_API bool state_when_t::has_handler() const  noexcept
		{
			return (bool)_coro;
		}

		LIBRF_API void state_when_t::on_cancel() noexcept
		{
			scoped_lock<lock_type> lock_(_lock);

			_counter.store(0);
			this->_coro = nullptr;
		}

		LIBRF_API bool state_when_t::on_notify_one()
		{
			scoped_lock<lock_type> lock_(_lock);

			if (_counter.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				assert(this->_scheduler != nullptr);
				if (this->_coro)
					this->_scheduler->add_generator(this);

				return true;
			}
			return false;
		}

		LIBRF_API bool state_when_t::on_timeout()
		{
			scoped_lock<lock_type> lock_(_lock);

			return false;
		}
	}
}
