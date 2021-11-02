#include "librf/librf.h"

namespace librf
{
	LIBRF_API task_t::task_t() noexcept
		: _stop(nostopstate)
	{
	}

	LIBRF_API task_t::~task_t()
	{
	}

	LIBRF_API const stop_source & task_t::get_stop_source()
	{
		_stop.make_sure_possible();
		return _stop;
	}
}
