#pragma once

RESUMEF_NS
{
	namespace detail
	{
		struct event_v2_impl;
	}

	namespace event_v2
	{
		struct event_t
		{
			using event_impl_ptr = std::shared_ptr<detail::event_v2_impl>;
			using clock_type = std::chrono::system_clock;

			event_t(bool initially = false);
			~event_t();

			void signal_all() const noexcept;
			void signal() const noexcept;
			void reset() const noexcept;

			struct [[nodiscard]] awaiter;
			awaiter operator co_await() const noexcept;
			awaiter wait() const noexcept;

			struct [[nodiscard]] timeout_awaiter;

			template<class _Rep, class _Period>
			timeout_awaiter wait_for(const std::chrono::duration<_Rep, _Period>& dt) const noexcept;
			template<class _Clock, class _Duration>
			timeout_awaiter wait_until(const std::chrono::time_point<_Clock, _Duration>& tp) const noexcept;
		private:
			event_impl_ptr _event;

			timeout_awaiter wait_until_(const clock_type::time_point& tp) const noexcept;
		};
	}

	template<class _Rep, class _Period>
	auto wait_for(event_v2::event_t& e, const std::chrono::duration<_Rep, _Period>& dt)
	{
		return e.wait_for(dt);
	}
			template<class _Clock, class _Duration>
	auto wait_until(event_v2::event_t& e, const std::chrono::time_point<_Clock, _Duration>& tp)
	{
		return e.wait_until(tp);
	}

	//when_all_for(dt, args...) -> when_all(wait_for(args, dt)...)
	//就不再单独为每个支持超时的类提供when_all_for实现了。借助when_all和非成员的wait_for实现
}
