#pragma once

namespace resumef
{
	struct scheduler;

	RF_API future_t<bool> sleep_until_(const std::chrono::system_clock::time_point& tp_, scheduler & scheduler_);

	inline future_t<bool> sleep_for_(const std::chrono::system_clock::duration& dt_, scheduler & scheduler_)
	{
		return std::move(sleep_until_(std::chrono::system_clock::now() + dt_, scheduler_));
	}

	template<class _Rep, class _Period>
	future_t<bool> sleep_for(const std::chrono::duration<_Rep, _Period>& dt_, scheduler & scheduler_)
	{
		return std::move(sleep_for_(std::chrono::duration_cast<std::chrono::system_clock::duration>(dt_), scheduler_));
	}
	template<class _Rep, class _Period>
	future_t<bool> sleep_for(const std::chrono::duration<_Rep, _Period>& dt_)
	{
		return std::move(sleep_for_(std::chrono::duration_cast<std::chrono::system_clock::duration>(dt_), *this_scheduler()));
	}

	template<class _Clock, class _Duration = typename _Clock::duration>
	future_t<bool> sleep_until(const std::chrono::time_point<_Clock, _Duration>& tp_, scheduler & scheduler_)
	{
		return std::move(sleep_until_(std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp_), scheduler_));
	}
	template<class _Clock, class _Duration>
	future_t<bool> sleep_until(const std::chrono::time_point<_Clock, _Duration>& tp_)
	{
		return std::move(sleep_until_(std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp_), *this_scheduler()));
	}
}