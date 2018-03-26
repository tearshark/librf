#include "when.h"
#include <assert.h>

namespace resumef
{
	namespace detail
	{
		when_impl::when_impl(intptr_t initial_counter_)
			: _counter(initial_counter_)
		{
		}

		void when_impl::signal()
		{
			scoped_lock<lock_type> lock_(this->_lock);

			if (--this->_counter == 0)
			{
				_awakes->awake(this, 1);
			}
		}

		void when_impl::reset(intptr_t initial_counter_)
		{
			scoped_lock<lock_type> lock_(this->_lock);

			this->_awakes = nullptr;
			this->_counter = initial_counter_;
		}

		bool when_impl::wait_(const when_awaker_ptr & awaker)
		{
			assert(awaker);

			scoped_lock<lock_type> lock_(this->_lock);
			if (this->_counter == 0)
			{
				awaker->awake(this, 1);
				return true;
			}
			else
			{
				this->_awakes = awaker;
				return false;
			}
		}
	}
}
