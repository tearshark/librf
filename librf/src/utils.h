#pragma once

RESUMEF_NS
{
	namespace traits
	{
		template<typename _Function>
		inline auto _IsCallable(_Function&& _Func, int) -> decltype(_Func(), std::true_type())
		{
			(_Func);
			return std::true_type();
		}
		template<typename _Function>
		inline std::false_type _IsCallable(_Function&&, ...)
		{
			return std::false_type();
		}

		template<typename _Function>
		using is_callable = decltype(_IsCallable(std::declval<_Function>(), 0));

		template<typename _Function>
		constexpr bool is_callable_v = is_callable<_Function>::value;
	}
}
