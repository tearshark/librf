#pragma once

namespace resumef
{
	enum struct error_code
	{
		none,
		not_ready,			// get_value called when value not available
		already_acquired,	// attempt to get another future
		unlock_more,		// unlock ��������lock����
		read_before_write,	// 0������channel���ȶ���д
		timer_canceled,		// ��ʱ��������ȡ��

		max__
	};

	const char* get_error_string(error_code fe, const char* classname);

	struct future_exception : std::exception
	{
		error_code _error;
		future_exception(error_code fe)
			: exception(get_error_string(fe, "future_exception"))
			, _error(fe)
		{
		}
	};

	struct lock_exception : std::exception
	{
		error_code _error;
		lock_exception(error_code fe)
			: exception(get_error_string(fe, "lock_exception"))
			, _error(fe)
		{
		}
	};

	struct channel_exception : std::exception
	{
		error_code _error;
		channel_exception(error_code fe)
			: exception(get_error_string(fe, "channel_exception"))
			, _error(fe)
		{
		}
	};

	struct timer_canceled_exception : public std::exception
	{
		error_code _error;
		timer_canceled_exception(error_code fe)
			: exception(get_error_string(fe, "timer canceled"))
			, _error(fe)
		{
		}
	};
}