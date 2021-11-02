#include "librf/librf.h"

namespace librf
{
	LIBRF_API future_t<> sleep_until_(std::chrono::system_clock::time_point tp_, scheduler_t& scheduler_)
	{
		awaitable_t<> awaitable;

		(void)scheduler_.timer()->add(tp_,
			[awaitable](bool cancellation_requested)
			{
				if (cancellation_requested)
					awaitable.throw_exception(canceled_exception{ error_code::timer_canceled });
				else
					awaitable.set_value();
			});

		return awaitable.get_future();
	}
}
