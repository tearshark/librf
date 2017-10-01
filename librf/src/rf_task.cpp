
#include "rf_task.h"
#include "scheduler.h"
#include <assert.h>

namespace resumef
{
	task_base::task_base()
	{
#if RESUMEF_DEBUG_COUNTER
		++g_resumef_task_count;
#endif
	}

	task_base::~task_base()
	{
#if RESUMEF_DEBUG_COUNTER
		--g_resumef_task_count;
#endif
	}

}
