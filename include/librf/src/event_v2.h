#pragma once

namespace librf
{
#ifndef DOXYGEN_SKIP_PROPERTY
	namespace detail
	{
		struct event_v2_impl;
	}
#endif	//DOXYGEN_SKIP_PROPERTY

	/**
		* @brief 用于协程的事件。
		* @details 用于同步不同线程里运行的协程。
		*/
	struct event_t
	{
		using event_impl_ptr = std::shared_ptr<detail::event_v2_impl>;
		using clock_type = std::chrono::system_clock;

		/**
			* @brief 构造一个事件。
			* @param initially 初始是否触发一次信号。
			*/
		LIBRF_API event_t(bool initially = false);

		/**
			* @brief 构造一个无效的事件。
			* @details 如果用于后续保存另外一个事件，则应当使用此构造函数，便于节省一次不必要的内部初始化。
			*/
		LIBRF_API event_t(std::adopt_lock_t);

		/**
			* @brief 采用shared_ptr<>来保存内部的事件实现。故不必担心正在被等待的协程，因为事件提前销毁而出现异常。
			*/
		LIBRF_API ~event_t();

		/**
			* @brief 向所有正在等待的协程触发一次信号。
			* @attention 非协程中也可以使用。
			*/
		void signal_all() const noexcept;

		/**
			* @brief 触发一次信号。
			* @details 如果有正在等待的协程，则最先等待的协程会被唤醒。\n
			* 如果没有正在等待的协程，则信号触发次数加一。之后有协程调用wait()，则会直接返回。
			* @attention 非协程中也可以使用。
			*/
		void signal() const noexcept;

		/**
			* @brief 重置信号。
			* @attention 非协程中也可以使用。
			*/
		void reset() const noexcept;

		struct [[nodiscard]] awaiter;

		/**
			* @brief 在协程中等待信号触发。
			* @see 等同于co_await wait()。
			* @attention 只能在协程中调用。
			*/
		awaiter operator co_await() const noexcept;

		/**
			* @brief 在协程中等待信号触发。
			* @details 如果信号已经触发，则立即返回true。\n
			* 否则，当前协程被阻塞，直到信号被触发后唤醒。
			* 消耗一次信号触发次数。
			* @retval bool [co_await] 返回是否等到了信号
			* @attention 只能在协程中调用。
			*/
		awaiter wait() const noexcept;

		template<class _Btype>
		struct timeout_awaitor_impl;

		struct [[nodiscard]] timeout_awaiter;

		/**
			* @brief 在协程中等待信号触发，直到超时。
			* @details 如果信号已经触发，则立即返回true。\n
			* 否则，当前协程被阻塞，直到信号被触发后，或者超时后唤醒。
			* 如果等到了信号，则消耗一次信号触发次数。
			* @param dt 超时时长
			* @retval bool [co_await] 等到了信号返回true，超时了返回false。
			* @attention 只能在协程中调用。
			*/
		template<class _Rep, class _Period>
		timeout_awaiter wait_for(const std::chrono::duration<_Rep, _Period>& dt) const noexcept;						//test OK

		/**
			* @brief 在协程中等待信号触发，直到超时。
			* @details 如果信号已经触发，则立即返回true。\n
			* 否则，当前协程被阻塞，直到信号被触发后，或者超时后唤醒。
			* 如果等到了信号，则消耗一次信号触发次数。
			* @param tp 超时时刻
			* @retval bool [co_await] 等到了信号返回true，超时了返回false。
			* @attention 只能在协程中调用。
			*/
		template<class _Clock, class _Duration>
		timeout_awaiter wait_until(const std::chrono::time_point<_Clock, _Duration>& tp) const noexcept;				//test OK


		template<class _Iter>
		struct [[nodiscard]] any_awaiter;

		template<class _Iter>
		requires(_IteratorOfT<_Iter, event_t>)
		static auto wait_any(_Iter begin_, _Iter end_)->any_awaiter<_Iter>;

		template<class _Cont>
		requires(_ContainerOfT<_Cont, event_t>)
		static auto wait_any(const _Cont& cnt_)->any_awaiter<decltype(std::begin(cnt_))>;

		template<class _Iter>
		struct [[nodiscard]] timeout_any_awaiter;

		template<class _Rep, class _Period, class _Iter>
		requires(_IteratorOfT<_Iter, event_t>)
		static auto wait_any_for(const std::chrono::duration<_Rep, _Period>& dt, _Iter begin_, _Iter end_)
			->timeout_any_awaiter<_Iter>;

		template<class _Rep, class _Period, class _Cont>
		requires(_ContainerOfT<_Cont, event_t>)
		static auto wait_any_for(const std::chrono::duration<_Rep, _Period>& dt, const _Cont& cnt_)
			->timeout_any_awaiter<decltype(std::begin(cnt_))>;



		template<class _Iter>
		struct [[nodiscard]] all_awaiter;

		template<class _Iter>
		requires(_IteratorOfT<_Iter, event_t>)
		static auto wait_all(_Iter begin_, _Iter end_)
			->all_awaiter<_Iter>;

		template<class _Cont>
		requires(_ContainerOfT<_Cont, event_t>)
		static auto wait_all(const _Cont& cnt_)
			->all_awaiter<decltype(std::begin(cnt_))>;


		template<class _Iter>
		struct [[nodiscard]] timeout_all_awaiter;

		template<class _Rep, class _Period, class _Iter>
		requires(_IteratorOfT<_Iter, event_t>)
		static auto wait_all_for(const std::chrono::duration<_Rep, _Period>& dt, _Iter begin_, _Iter end_)
			->timeout_all_awaiter<_Iter>;

		template<class _Rep, class _Period, class _Cont>
		requires(_ContainerOfT<_Cont, event_t>)
		static auto wait_all_for(const std::chrono::duration<_Rep, _Period>& dt, const _Cont& cnt_)
			->timeout_all_awaiter<decltype(std::begin(cnt_))>;

		event_t(const event_t&) = default;
		event_t(event_t&&) = default;
		event_t& operator = (const event_t&) = default;
		event_t& operator = (event_t&&) = default;
	private:
		event_impl_ptr _event;
	};
}
