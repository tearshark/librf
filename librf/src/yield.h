#pragma once

namespace resumef
{
	/**
	 * @brief 将本协程让渡出一次调用的可等待对象。
	 */
	struct yield_awaitor
	{
		using value_type = void;
		using state_type = state_t<value_type>;
		using promise_type = promise_t<value_type>;
		using lock_type = typename state_type::lock_type;

		bool await_ready() const noexcept
		{
			return false;
		}
		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		bool await_suspend(coroutine_handle<_PromiseT> handler)
		{
			counted_ptr<state_t<void>> _state = state_future_t::_Alloc_state<state_type>(true);
			_state->set_value();
			_state->future_await_suspend(handler);

			return true;
		}
		void await_resume() const noexcept
		{
		}

#ifdef DOXYGEN_SKIP_PROPERTY
		/**
		 * @brief 将本协程让渡出一次调用的可等待对象。
		 * @return [co_await] void
		 * @note 本函数是resumef名字空间下的全局函数。由于doxygen使用上的问题，将之归纳到 yield_awaitor 类下。
		 */
		static yield_awaitor yield() noexcept;
#endif	//DOXYGEN_SKIP_PROPERTY
	};

	/**
	 * @brief 将本协程让渡出一次调用。
	 * @return [co_await] void
	 */
	inline yield_awaitor yield() noexcept
	{
		return {};
	}

}
