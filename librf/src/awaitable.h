#pragma once

RESUMEF_NS
{
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

		void set_exception(std::exception_ptr e) const
		{
			this->_state->set_exception(std::move(e));
			this->_state = nullptr;
		}

		template<class _Exp>
		void throw_exception(_Exp e) const
		{
			set_exception(std::make_exception_ptr(std::move(e)));
		}

		future_type get_future()
		{
			return future_type{ this->_state };
		}

		mutable counted_ptr<state_type> _state = state_future_t::_Alloc_state<state_type>(true);
	};

	template<class _Ty>
	struct awaitable_t : public awaitable_impl_t<_Ty>
	{
		using typename awaitable_impl_t<_Ty>::value_type;
		using awaitable_impl_t<_Ty>::awaitable_impl_t;

		template<class U>
		void set_value(U&& value) const
		{
			this->_state->set_value(std::forward<U>(value));
			this->_state = nullptr;
		}
	};

	template<>
	struct awaitable_t<void> : public awaitable_impl_t<void>
	{
		using awaitable_impl_t<void>::awaitable_impl_t;

		void set_value() const
		{
			this->_state->set_value();
			this->_state = nullptr;
		}
	};
}
