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

			event_t(bool initially = false);
			~event_t();

			void signal_all() const noexcept;
			void signal() const noexcept;
			void reset() const noexcept;

			struct [[nodiscard]] awaiter;
			awaiter operator co_await() const noexcept;
			awaiter wait() const noexcept;
		private:
			event_impl_ptr _event;
		};
	}
}
