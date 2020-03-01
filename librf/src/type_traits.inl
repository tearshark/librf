#pragma once

RESUMEF_NS
{
	template<class _PromiseT>
	struct is_promise : std::false_type {};
	template<class _Ty>
	struct is_promise<promise_t<_Ty>> : std::true_type {};
	template<class _Ty>
	constexpr bool is_promise_v = is_promise<remove_cvref_t<_Ty>>::value;

	template<class _PromiseT>
	struct is_future : std::false_type {};
	template<class _Ty>
	struct is_future<future_t<_Ty>> : std::true_type {};
	template<class _Ty>
	constexpr bool is_future_v = is_future<remove_cvref_t<_Ty>>::value;

	template<class _G>
	struct is_generator : std::false_type {};
	template <typename _Ty, typename _Alloc>
	struct is_generator<generator_t<_Ty, _Alloc>> : std::true_type {};
	template<class _Ty>
	constexpr bool is_generator_v = is_generator<remove_cvref_t<_Ty>>::value;

	template<class _PromiseT>
	struct is_awaitable : std::false_type {};
	template<class _Ty>
	struct is_awaitable<awaitable_t<_Ty>> : std::true_type {};
	template<class _Ty>
	constexpr bool is_awaitable_v = is_awaitable<remove_cvref_t<_Ty>>::value;

	template<class _Ty>
	constexpr bool is_await_suspend_v = is_future_v<_Ty> 
									|| is_generator_v<_Ty> 
									|| is_awaitable_v<_Ty> 
									|| std::is_same_v<remove_cvref_t<_Ty>, switch_scheduler_t>
		;
}
