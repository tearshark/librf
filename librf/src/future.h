
#pragma once

namespace resumef
{

	/**
	 * @brief 用于resumef协程的返回值。
	 * @details 由于coroutines的限制，协程的返回值必须明确申明，而不能通过auto推导。\n
	 * 用在恢复函数(resumeable function)里，支持co_await和co_yield。\n
	 * 用在可等待函数(awaitable function)里，与awaitable_t<>配套使用。
	 */
	template<class _Ty>
	struct [[nodiscard]] future_t
	{
		using value_type = _Ty;
		using state_type = state_t<value_type>;
		using promise_type = promise_t<value_type>;
		using future_type = future_t<value_type>;
		using lock_type = typename state_type::lock_type;

		counted_ptr<state_type> _state;

		future_t(counted_ptr<state_type> _st) noexcept
			:_state(std::move(_st)) {}
		future_t(const future_t&) = default;
		future_t(future_t&&) = default;

		future_t& operator = (const future_t&) = default;
		future_t& operator = (future_t&&) = default;

		bool await_ready() noexcept
		{
			return _state->future_await_ready();
		}

		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		void await_suspend(coroutine_handle<_PromiseT> handler)
		{
			_state->future_await_suspend(handler);
		}

		value_type await_resume()
		{
			return _state->future_await_resume();
		}
	};
}

#ifndef DOXYGEN_SKIP_PROPERTY
namespace std {
	namespace experimental {

		/*If the coroutine is defined as task<float> foo(std::string x, bool flag);, 
		then its Promise type is std::coroutine_traits<task<float>, std::string, bool>::promise_type.
		If the coroutine is a non-static member function, such as task<void> my_class::method1(int x) const;, 
		its Promise type is std::coroutine_traits<task<void>, const my_class&, int>::promise_type.
		*/
		template <typename _Ty, typename... Args>
		struct coroutine_traits<resumef::future_t<_Ty>, Args...>
		{
			typedef resumef::promise_t<_Ty> promise_type;
		};
	}
} // namespace std::experimental
#endif	//DOXYGEN_SKIP_PROPERTY
