#pragma once

RESUMEF_NS
{
	namespace detail
	{
		struct event_v2_impl;
	}

	inline namespace event_v2
	{
		struct event_t
		{
			using event_impl_ptr = std::shared_ptr<detail::event_v2_impl>;
			using clock_type = std::chrono::system_clock;

			event_t(bool initially = false);
			event_t(std::adopt_lock_t);
			~event_t();

			void signal_all() const noexcept;
			void signal() const noexcept;
			void reset() const noexcept;

			struct [[nodiscard]] awaiter;

			awaiter operator co_await() const noexcept;
			awaiter wait() const noexcept;

			template<class _Btype>
			struct timeout_awaitor_impl;

			struct [[nodiscard]] timeout_awaiter;

			template<class _Rep, class _Period>
			timeout_awaiter wait_for(const std::chrono::duration<_Rep, _Period>& dt) const noexcept;
			template<class _Clock, class _Duration>
			timeout_awaiter wait_until(const std::chrono::time_point<_Clock, _Duration>& tp) const noexcept;


			//以下wait_any/wait_all实现，借助when_any/when_all实现。
			//其接口尽管兼容了v1版本的event_t，但是，其内部细节并没有兼容
			//v1版本的event_t的wait_any，确保只触发了其中之一，或者超时
			//而when_any会导致所有的event_t都被触发
			//改日有空再补上

			template<class _Iter>
			struct [[nodiscard]] any_awaiter;

			template<class _Iter
				COMMA_RESUMEF_ENABLE_IF(traits::is_iterator_of_v<_Iter, event_t>)
			> RESUMEF_REQUIRES(_IteratorOfT<_Iter, event_t>)
			static auto wait_any(_Iter begin_, _Iter end_)
				->any_awaiter<_Iter>;

			template<class _Cont
				COMMA_RESUMEF_ENABLE_IF(traits::is_container_of_v<_Cont, event_t>)
			> RESUMEF_REQUIRES(_ContainerOfT<_Cont, event_t>)
			static auto wait_any(_Cont& cnt_)
				->any_awaiter<decltype(std::begin(cnt_))>;

			template<class _Iter>
			struct [[nodiscard]] timeout_any_awaiter;

			template<class _Rep, class _Period, class _Iter
				COMMA_RESUMEF_ENABLE_IF(traits::is_iterator_of_v<_Iter, event_t>)
			> RESUMEF_REQUIRES(_IteratorOfT<_Iter, event_t>)
			static auto wait_any_for(const std::chrono::duration<_Rep, _Period>& dt, _Iter begin_, _Iter end_)
				->timeout_any_awaiter<_Iter>;

			template<class _Rep, class _Period, class _Cont
				COMMA_RESUMEF_ENABLE_IF(traits::is_container_of_v<_Cont, event_t>)
			> RESUMEF_REQUIRES(_ContainerOfT<_Cont, event_t>)
			static auto wait_any_for(const std::chrono::duration<_Rep, _Period>& dt, _Cont& cnt_)
				->timeout_any_awaiter<decltype(std::begin(cnt_))>;



			template<class _Iter>
			struct [[nodiscard]] all_awaiter;

			template<class _Iter
				COMMA_RESUMEF_ENABLE_IF(traits::is_iterator_of_v<_Iter, event_t>)
			> RESUMEF_REQUIRES(_IteratorOfT<_Iter, event_t>)
			static auto wait_all(_Iter begin_, _Iter end_)
				->all_awaiter<_Iter>;

			template<class _Cont
				COMMA_RESUMEF_ENABLE_IF(traits::is_container_of_v<_Cont, event_t>)
			> RESUMEF_REQUIRES(_ContainerOfT<_Cont, event_t>)
			static auto wait_all(_Cont& cnt_)
				->all_awaiter<decltype(std::begin(cnt_))>;

			template<class _Iter>
			struct [[nodiscard]] timeout_all_awaiter;

			template<class _Rep, class _Period, class _Iter
				COMMA_RESUMEF_ENABLE_IF(traits::is_iterator_of_v<_Iter, event_t>)
			> RESUMEF_REQUIRES(_IteratorOfT<_Iter, event_t>)
			static auto wait_all_for(const std::chrono::duration<_Rep, _Period>& dt, _Iter begin_, _Iter end_)
				->timeout_all_awaiter<_Iter>;

			template<class _Rep, class _Period, class _Cont
				COMMA_RESUMEF_ENABLE_IF(traits::is_container_of_v<_Cont, event_t>)
			> RESUMEF_REQUIRES(_ContainerOfT<_Cont, event_t>)
			static auto wait_all_for(const std::chrono::duration<_Rep, _Period>& dt, _Cont& cnt_)
				->timeout_all_awaiter<decltype(std::begin(cnt_))>;

			event_t(const event_t&) = default;
			event_t(event_t&&) = default;
			event_t& operator = (const event_t&) = default;
			event_t& operator = (event_t&&) = default;
		private:
			//friend struct any_awaiter;

			event_impl_ptr _event;

			timeout_awaiter wait_until_(const clock_type::time_point& tp) const noexcept;
			inline static bool is_all_succeeded(const std::vector<bool>& v)
			{
				return std::none_of(std::begin(v), std::end(v), [](auto v) 
					{
						return v == false;
					});
			}
		};
	}
}
