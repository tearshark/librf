
#include "rf_task.h"
#include "scheduler.h"
#include <assert.h>

namespace resumef
{
	task_base_t::task_base_t()
		: _next_node(nullptr)
		, _prev_node(nullptr)
	{
	}

	task_base_t::~task_base_t()
	{
	}
}
