#include "../librf.h"

namespace resumef
{
	task_t::task_t()
		: _stop(nostopstate)
	{
	}

	task_t::~task_t()
	{
	}

	const stop_source & task_t::get_stop_source()
	{
		_stop.make_sure_possible();
		return _stop;
	}
}
