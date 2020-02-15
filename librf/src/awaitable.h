#pragma once

namespace resumef
{
	template<class _Ty>
	struct awaitable_t
	{
		using value_type = _Ty;
		using state_type = state_t<value_type>;
		using future_type = future_t<value_type>;
		using lock_type = typename state_type::lock_type;

	private:
		mutable counted_ptr<state_type> _state = make_counted<state_type>(true);
	public:
		awaitable_t() {}
		awaitable_t(const awaitable_t&) = default;
		awaitable_t(awaitable_t&&) = default;

		awaitable_t& operator = (const awaitable_t&) = default;
		awaitable_t& operator = (awaitable_t&&) = default;

		void set_value(value_type value) const
		{
			_state->set_value(std::move(value));
			_state = nullptr;
		}

		void set_exception(std::exception_ptr e)
		{
			_state->set_exception(std::move(e));
			_state = nullptr;
		}

		future_type get_future()
		{
			return future_type{ _state };
		}
	};

	template<>
	struct awaitable_t<void>
	{
		using value_type = void;
		using state_type = state_t<void>;
		using future_type = future_t<void>;
		using lock_type = typename state_type::lock_type;

		mutable counted_ptr<state_type> _state = make_counted<state_type>(true);

		awaitable_t() {}
		awaitable_t(const awaitable_t&) = default;
		awaitable_t(awaitable_t&&) = default;

		awaitable_t& operator = (const awaitable_t&) = default;
		awaitable_t& operator = (awaitable_t&&) = default;

		void set_value() const
		{
			_state->set_value();
			_state = nullptr;
		}

		void set_exception(std::exception_ptr e)
		{
			_state->set_exception(std::move(e));
			_state = nullptr;
		}

		future_type get_future()
		{
			return future_type{ _state };
		}
	};
}
