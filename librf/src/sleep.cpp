
#include "scheduler.h"
#include "sleep.h"

namespace resumef
{
	future_t<> sleep_until_(const std::chrono::system_clock::time_point& tp_, scheduler_t& scheduler_)
	{
		promise_vt awaitable;

		(void)scheduler_.timer()->add(tp_,
			[st = awaitable._state](bool cancellation_requested)
			{
				if (cancellation_requested)
					st->throw_exception(timer_canceled_exception{ error_code::timer_canceled });
				else
					st->set_value();
			});

		return awaitable.get_future();
	}
}