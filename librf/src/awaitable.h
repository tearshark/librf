#pragma once

namespace resumef
{
	template<class _Ty>
	struct awaitable_impl_t
	{
		using value_type = _Ty;
		using state_type = state_t<value_type>;
		using future_type = future_t<value_type>;
		using lock_type = typename state_type::lock_type;

		awaitable_impl_t() {}
		awaitable_impl_t(const awaitable_impl_t&) = default;
		awaitable_impl_t(awaitable_impl_t&&) = default;

		awaitable_impl_t& operator = (const awaitable_impl_t&) = default;
		awaitable_impl_t& operator = (awaitable_impl_t&&) = default;

		void set_exception(std::exception_ptr e) const
		{
			_state->set_exception(std::move(e));
			_state = nullptr;
		}

		template<class _Exp>
		void throw_exception(_Exp e) const
		{
			set_exception(std::make_exception_ptr(std::move(e)));
		}

		future_type get_future()
		{
			return future_type{ _state };
		}
	protected:
		mutable counted_ptr<state_type> _state = make_counted<state_type>(true);
	};

	template<class _Ty>
	struct awaitable_t : public awaitable_impl_t<_Ty>
	{
		using awaitable_impl_t::awaitable_impl_t;

		void set_value(value_type value) const
		{
			_state->set_value(std::move(value));
			_state = nullptr;
		}
	};

	template<>
	struct awaitable_t<void> : public awaitable_impl_t<void>
	{
		using awaitable_impl_t::awaitable_impl_t;

		void set_value() const
		{
			_state->set_value();
			_state = nullptr;
		}
	};
}
