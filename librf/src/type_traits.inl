#pragma once

RESUMEF_NS
{
	namespace traits
	{
		template<class _Ty>
		struct is_coroutine_handle : std::false_type {};
		template<class _PromiseT>
		struct is_coroutine_handle<std::experimental::coroutine_handle<_PromiseT>> : std::true_type {};
		template<class _Ty>
		constexpr bool is_coroutine_handle_v = is_coroutine_handle<remove_cvref_t<_Ty>>::value;

		template<class _Ty>
		constexpr bool is_valid_await_suspend_return_v = std::is_void_v<_Ty> || std::is_same_v<_Ty, bool> || is_coroutine_handle_v<_Ty>;

		template<class _Ty, class = std::void_t<>>
		struct is_awaitor : std::false_type {};
		template<class _Ty>
		struct is_awaitor
			<_Ty,
				std::void_t<
					decltype(std::declval<_Ty>().await_ready())
					, decltype(std::declval<_Ty>().await_suspend(std::declval<std::experimental::coroutine_handle<promise_t<>>>()))
					, decltype(std::declval<_Ty>().await_resume())
				>
			>
			: std::bool_constant<
				std::is_constructible_v<bool, decltype(std::declval<_Ty>().await_ready())>
				&& is_valid_await_suspend_return_v<
					decltype(std::declval<_Ty>().await_suspend(std::declval<std::experimental::coroutine_handle<promise_t<>>>()))
				>
			>
		{};
		template<class _Ty>
		struct is_awaitor<_Ty&> : is_awaitor<_Ty> {};
		template<class _Ty>
		struct is_awaitor<_Ty&&> : is_awaitor<_Ty> {};

		template<class _Ty>
		constexpr bool is_awaitor_v = is_awaitor<remove_cvref_t<_Ty>>::value;

		template<class _Ty, class = std::void_t<>>
		struct is_future : std::false_type {};
		template<class _Ty>
		struct is_future<_Ty,
				std::void_t<
					decltype(std::declval<_Ty>()._state),
					typename _Ty::value_type,
					typename _Ty::state_type,
					typename _Ty::promise_type
				>
			> : std::true_type {};
		template<class _Ty>
		constexpr bool is_future_v = is_future<remove_cvref_t<_Ty>>::value;

		template<class _Ty>
		struct is_promise : std::false_type {};
		template<class _Ty>
		struct is_promise<promise_t<_Ty>> : std::true_type {};
		template<class _Ty>
		constexpr bool is_promise_v = is_promise<remove_cvref_t<_Ty>>::value;

		template<class _Ty>
		struct is_generator : std::false_type {};
		template <class _Ty, class _Alloc>
		struct is_generator<generator_t<_Ty, _Alloc>> : std::true_type {};
		template<class _Ty>
		constexpr bool is_generator_v = is_generator<remove_cvref_t<_Ty>>::value;

		template<class _Ty, class = std::void_t<>>
		struct has_state : std::false_type {};
		template<class _Ty>
		struct has_state<_Ty, std::void_t<decltype(std::declval<_Ty>()._state)>> : std::true_type {};
		template<class _Ty>
		constexpr bool has_state_v = has_state<remove_cvref_t<_Ty>>::value;


		//copy from cppcoro
		namespace detail
		{
			template<class T>
			auto get_awaitor_impl(T&& value, int) noexcept(noexcept(static_cast<T&&>(value).operator co_await()))
				-> decltype(static_cast<T&&>(value).operator co_await())
			{
				return static_cast<T&&>(value).operator co_await();
			}
			template<class T>
			auto get_awaitor_impl(T&& value, long) noexcept(noexcept(operator co_await(static_cast<T&&>(value))))
				-> decltype(operator co_await(static_cast<T&&>(value)))
			{
				return operator co_await(static_cast<T&&>(value));
			}
			template<class T, std::enable_if_t<is_awaitor_v<T&&>, int> = 0>
			T&& get_awaitor_impl(T&& value, std::any) noexcept
			{
				return static_cast<T&&>(value);
			}
		}

		template<class T>
		auto get_awaitor(T&& value) noexcept(noexcept(detail::get_awaitor_impl(static_cast<T&&>(value), 123)))
			-> decltype(detail::get_awaitor_impl(static_cast<T&&>(value), 123))
		{
			return detail::get_awaitor_impl(static_cast<T&&>(value), 123);
		}
}
}
