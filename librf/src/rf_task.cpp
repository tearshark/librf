#include "../librf.h"

namespace resumef
{
	task_t::task_t()
		: _stop(nostopstate)
	{
	}

	task_t::~task_t()
	{
		///TODO : 这里有线程安全问题(2020/05/09)
		_stop.clear_callback();
		///TODO : 这里有线程安全问题(2020/05/09)
	}

	const stop_source & task_t::get_stop_source()
	{
		///TODO : 这里有线程安全问题(2020/05/09)
		_stop.make_possible();
		///TODO : 这里有线程安全问题(2020/05/09)

		return _stop;
	}
}
