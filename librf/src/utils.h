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
}

