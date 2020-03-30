#pragma once

namespace resumef
{

	/**
	 * @brief awaitable_t<>的公共实现部分，用于减少awaitable_t<>的重复代码。
	 * @param _Ty 可等待函数(awaitable function)的返回类型。
	 * @see 参见awaitable_t<>类的说明。
	 */
	template<class _Ty>
	struct awaitable_impl_t
	{
		using value_type = _Ty;
		using state_type = state_t<value_type>;
		using future_type = future_t<value_type>;
		using lock_type = typename state_type::lock_type;
		using _Alloc_char = typename state_type::_Alloc_char;

		awaitable_impl_t() {}
		awaitable_impl_t(const awaitable_impl_t&) = default;
		awaitable_impl_t(awaitable_impl_t&&) = default;

		awaitable_impl_t& operator = (const awaitable_impl_t&) = default;
		awaitable_impl_t& operator = (awaitable_impl_t&&) = default;

		/**
		 * @brief 发生了异常后，设置异常。
		 * @attention 与set_value()互斥。调用了set_exception(e)后，不能再调用set_value()。
		 */
		void set_exception(std::exception_ptr e) const
		{
			this->_state->set_exception(std::move(e));
			this->_state = nullptr;
		}

		/**
		 * @brief 在协程内部，重新抛出之前设置的异常。
		 */
		template<class _Exp>
		void throw_exception(_Exp e) const
		{
			set_exception(std::make_exception_ptr(std::move(e)));
		}

		/**
		 * @brief 获得与之关联的future_t<>对象，作为可等待函数(awaitable function)的返回值。
		 */
		future_type get_future() noexcept
		{
			return future_type{ this->_state };
		}

		/**
		 * @brief 管理的state_t<>对象。
		 */
		mutable counted_ptr<state_type> _state = state_future_t::_Alloc_state<state_type>(true);
	};

	/**
	 * @brief 用于包装‘异步函数’为‘可等待函数(awaitable function)’。
	 * @details 通过返回一个‘可等待对象(awaitor)’，符合C++ coroutines的co_await所需的接口，来达成‘可等待函数(awaitable function)’。\n
	 * 这是扩展异步函数支持协程的重要手段。\n
	 * \n
	 * 典型用法是申明一个 awaitable_t<>局部变量 awaitable，\n
	 * 在已经获得结果的情况下，直接调用 awaitable.set_value(value)设置返回值，使得可等待函数立即获得结果。\n
	 * 在不能立即获得结果的情况下，通过在异步的回调lambda里，捕获awaitable局部变量，\n
	 * 根据异步结果，要么调用 awaitable.set_value(value)设置结果值，要么调用 awaitable.set_exception(e)设置异常。\n
	 * 在设置值或者异常后，调用可等待函数的协程将得以继续执行。\n
	 * 此可等待函数通过 awaitable.get_future()返回与之关联的 future_t<>对象，作为协程的可等待对象。\n
	 * \n
	 * @param _Ty 可等待函数(awaitable function)的返回类型。\n
	 * 要求至少支持移动构造和移动赋值。\n
	 * _Ty 支持特化为 _Ty&，以及 void。
	 */
	template<class _Ty>
	struct [[nodiscard]] awaitable_t : public awaitable_impl_t<_Ty>
	{
		using typename awaitable_impl_t<_Ty>::value_type;
		using awaitable_impl_t<_Ty>::awaitable_impl_t;

		/**
		 * @brief 设置可等待函数的返回值。
		 * @details _Ty的void特化版本，则是不带参数的set_value()函数。
		 * @param value 返回值。必须支持通过value构造出_Ty类型。
		 * @attention 与set_exception()互斥。调用了set_value(value)后，不能再调用set_exception(e)。
		 */
		template<class U>
		void set_value(U&& value) const
		{
			this->_state->set_value(std::forward<U>(value));
			this->_state = nullptr;
		}
	};

#ifndef DOXYGEN_SKIP_PROPERTY
	template<class _Ty>
	struct [[nodiscard]] awaitable_t<_Ty&> : public awaitable_impl_t<_Ty&>
	{
		using typename awaitable_impl_t<_Ty&>::value_type;
		using awaitable_impl_t<_Ty&>::awaitable_impl_t;

		void set_value(_Ty& value) const
		{
			this->_state->set_value(value);
			this->_state = nullptr;
		}
	};

	template<>
	struct [[nodiscard]] awaitable_t<void> : public awaitable_impl_t<void>
	{
		using awaitable_impl_t<void>::awaitable_impl_t;

		void set_value() const
		{
			this->_state->set_value();
			this->_state = nullptr;
		}
	};
#endif	//DOXYGEN_SKIP_PROPERTY
}
