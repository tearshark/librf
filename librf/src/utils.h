#pragma once

namespace std
{
	template<typename _Function>
	inline auto _IsCallable(const _Function & _Func, int) -> decltype(_Func(), true_type())
	{
		(_Func);
		return true_type();
	}
	template<typename _Function>
	inline false_type _IsCallable(const _Function &, ...)
	{
		return false_type();
	}

	template<typename _Function>
	using is_callable = decltype(_IsCallable(std::declval<_Function>(), 0));

	template<typename _Function>
	_INLINE_VAR constexpr bool is_callable_v = is_callable<_Function>::value;
}

