//协程的定时器
//如果定时器被取消了，则会抛 timer_canceled_exception 异常
//不使用co_await而单独sleep_for/sleep_until，是错误的用法，并不能达到等待的目的。而仅仅是添加了一个无效的定时器，造成无必要的内存使用
//
#pragma once

namespace resumef
{
	/**
	 * @brief 协程专用的睡眠功能。
	 * @details 不能使用操作系统提供的sleep功能，因为会阻塞协程。\n
	 * 此函数不会阻塞线程，仅仅将当前协程挂起，直到指定时刻。\n
	 * 其精度，取决与调度器循环的精度，以及std::chrono::system_clock的精度。简而言之，可以认为只要循环够快，精度到100ns。
	 * @return [co_await] void
	 * @throw timer_canceled_exception 如果定时器被取消，则抛此异常。
	 */
	future_t<> sleep_until_(std::chrono::system_clock::time_point tp_, scheduler_t& scheduler_);

	/**
	 * @brief 协程专用的睡眠功能。
	 * @see 参考sleep_until_()函数\n
	 * @return [co_await] void
	 * @throw timer_canceled_exception 如果定时器被取消，则抛此异常。
	 */
	inline future_t<> sleep_for_(std::chrono::system_clock::duration dt_, scheduler_t& scheduler_)
	{
		return sleep_until_(std::chrono::system_clock::now() + dt_, scheduler_);
	}

	/**
	 * @brief 协程专用的睡眠功能。
	 * @see 参考sleep_until_()函数\n
	 * @return [co_await] void
	 * @throw timer_canceled_exception 如果定时器被取消，则抛此异常。
	 */
	template<class _Rep, class _Period>
	inline future_t<> sleep_for(std::chrono::duration<_Rep, _Period> dt_, scheduler_t& scheduler_)
	{
		return sleep_for_(std::chrono::duration_cast<std::chrono::system_clock::duration>(dt_), scheduler_);
	}

	/**
	 * @brief 协程专用的睡眠功能。
	 * @see 参考sleep_until_()函数\n
	 * @return [co_await] void
	 * @throw timer_canceled_exception 如果定时器被取消，则抛此异常。
	 */
	template<class _Clock, class _Duration = typename _Clock::duration>
	inline future_t<> sleep_until(std::chrono::time_point<_Clock, _Duration> tp_, scheduler_t& scheduler_)
	{
		return sleep_until_(std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp_), scheduler_);
	}

	/**
	 * @brief 协程专用的睡眠功能。
	 * @see 参考sleep_until_()函数\n
	 * @return [co_await] void
	 * @throw timer_canceled_exception 如果定时器被取消，则抛此异常。
	 */
	template<class _Rep, class _Period>
	inline future_t<> sleep_for(std::chrono::duration<_Rep, _Period> dt_)
	{
		scheduler_t* sch = current_scheduler();
		co_await sleep_for_(std::chrono::duration_cast<std::chrono::system_clock::duration>(dt_), *sch);
	}

	/**
	 * @brief 协程专用的睡眠功能。
	 * @see 参考sleep_until_()函数\n
	 * @return [co_await] void
	 * @throw timer_canceled_exception 如果定时器被取消，则抛此异常。
	 */
	template<class _Clock, class _Duration>
	inline future_t<> sleep_until(std::chrono::time_point<_Clock, _Duration> tp_)
	{
		scheduler_t* sch = current_scheduler();
		co_await sleep_until_(std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp_), *sch);
	}

	/**
	 * @brief 协程专用的睡眠功能。
	 * @see 等同调用sleep_for(dt)\n
	 * @return [co_await] void
	 * @throw timer_canceled_exception 如果定时器被取消，则抛此异常。
	 */
	template <class Rep, class Period>
	inline future_t<> operator co_await(std::chrono::duration<Rep, Period> dt_)
	{
		scheduler_t* sch = current_scheduler();
		co_await sleep_for(dt_, *sch);
	}

}