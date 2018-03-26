
#include "scheduler.h"
#include "sleep.h"

namespace resumef
{

	future_t<bool> sleep_until_(const std::chrono::system_clock::time_point& tp_, scheduler & scheduler_)
	{
		promise_t<bool> awaitable;

		scheduler_.timer()->add(tp_, 
			[st = awaitable._state](bool cancellation_requested)
			{
				st->set_value(cancellation_requested);
			});

		return awaitable.get_future();
	}
}