#pragma once

namespace resumef
{

	/**
	 * @brief 错误码。
	 */
	enum struct error_code
	{
		none,
		not_ready,			///< get_value called when value not available
		already_acquired,	///< attempt to get another future
		unlock_more,		///< unlock 次数多余lock次数
		read_before_write,	///< 0容量的channel，先读后写
		timer_canceled,		///< 定时器被意外取消
		not_await_lock,		///< 没有在协程中使用co_await等待lock结果

		max__
	};

	/**
	 * @brief 通过错误码获得错误描述字符串。
	 */
	const char* get_error_string(error_code fe, const char* classname);

	/**
	 * @brief 在操作future_t<>时产生的异常。
	 */
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

	/**
	 * @brief 错误使用mutex_t时产生的异常。
	 */
	struct mutex_exception : std::logic_error
	{
		error_code _error;
		mutex_exception(error_code fe)
			: logic_error(get_error_string(fe, "mutex_exception"))
			, _error(fe)
		{
		}
	};

	/**
	 * @brief 错误使用channel_t时产生的异常(v2版本已经不再抛此异常了）。
	 */
	struct channel_exception : std::logic_error
	{
		error_code _error;
		channel_exception(error_code fe)
			: logic_error(get_error_string(fe, "channel_exception"))
			, _error(fe)
		{
		}
	};

	/**
	 * @brief 定时器提前取消导致的异常。
	 */
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