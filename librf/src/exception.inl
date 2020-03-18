#pragma once

RESUMEF_NS
{
	enum struct error_code
	{
		none,
		not_ready,			// get_value called when value not available
		already_acquired,	// attempt to get another future
		unlock_more,		// unlock 次数多余lock次数
		read_before_write,	// 0容量的channel，先读后写
		timer_canceled,		// 定时器被意外取消
		not_await_lock,		// 没有在协程中使用co_await等待lock结果

		max__
	};

	const char* get_error_string(error_code fe, const char* classname);

	struct future_exception : std::logic_error
	{
		error_code _error;
		future_exception(error_code fe)
			: logic_error(get_error_string(fe, "future_exception"))
			, _error(fe)
		{
		}
	};

	struct lock_exception : std::logic_error
	{
		error_code _error;
		lock_exception(error_code fe)
			: logic_error(get_error_string(fe, "lock_exception"))
			, _error(fe)
		{
		}
	};

	struct channel_exception : std::logic_error
	{
		error_code _error;
		channel_exception(error_code fe)
			: logic_error(get_error_string(fe, "channel_exception"))
			, _error(fe)
		{
		}
	};

	struct timer_canceled_exception : public std::logic_error
	{
		error_code _error;
		timer_canceled_exception(error_code fe)
			: logic_error(get_error_string(fe, "timer canceled"))
			, _error(fe)
		{
		}
	};
}