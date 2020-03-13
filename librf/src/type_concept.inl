#pragma once
#include <concepts>

RESUMEF_NS
{
#define RESUMEF_ENABLE_CONCEPT	1

#if RESUMEF_ENABLE_CONCEPT

	template<typename T>
	concept _AwaitorT = requires(T && v)
	{
		{ v.await_ready() } -> bool;
		{ v.await_suspend(std::declval<std::experimental::coroutine_handle<promise_t<>>>()) };
		{ v.await_resume() };
	};

	template<typename T> 
	concept _HasStateT = _AwaitorT<T> && requires(T && v)
	{
		{ v._state };
	};

	template<typename T>
	concept _FutureT = _AwaitorT<T> && _HasStateT<T> && requires
	{
		{ T::value_type };
		{ T::state_type };
		{ T::promise_type };
	};

	template<typename T>
	concept _CallableT = std::invocable<T>;

	template<typename T>
	concept _GeneratorT = std::is_same_v<T, generator_t<_Ty>>;

	template<typename T>
	concept _WhenTaskT = _AwaitorT<T> || _CallableT<T>;

	template<typename T>
	concept _WhenIterT = requires(T&& u, T&& v)
	{
		{ ++u } -> T;
		{ u != v } -> bool;
		{ *u };
		//requires _WhenTaskT<*u>;
	};

#define COMMA_RESUMEF_ENABLE_IF(...) ,typename=std::enable_if_t<__VA_ARGS__>
#define RESUMEF_ENABLE_IF(...) typename=std::enable_if_t<__VA_ARGS__>
#define RESUMEF_REQUIRES(...) requires __VA_ARGS__

#else

#define _AwaitorT typename
#define _HasStateT typename
#define _FutureT typename
#define _CallableT typename
#define _GeneratorT typename
#define _WhenTaskT typename
#define _WhenIterT typename

#define COMMA_RESUMEF_ENABLE_IF(...) ,typename=std::enable_if_t<__VA_ARGS__>
#define RESUMEF_ENABLE_IF(...) typename=std::enable_if_t<__VA_ARGS__>
#define RESUMEF_REQUIRES(...) 

#endif

}
