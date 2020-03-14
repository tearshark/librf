#pragma once

RESUMEF_NS
{
	namespace traits
	{
		//is_coroutine_handle<T>
		//is_coroutine_handle_v<T>
		//判断是不是coroutine_handle<>类型
		//
		//is_valid_await_suspend_return_v<T>
		//判断是不是awaitor的await_suspend()函数的有效返回值
		//
		//is_awaitor<T>
		//is_awaitor_v<T>
		//判断是不是一个awaitor规范。
		//一个awaitor可以被co_await操作，要求满足coroutine的awaitor的三个函数接口规范
		//
		//is_future<T>
		//is_future_v<T>
		//判断是不是一个librf的future规范。
		//future除了要求是一个awaitor外，还要求定义了value_type/state_type/promise_type三个类型，
		//并且具备counted_ptr<state_type>类型的_state变量。
		//
		//is_promise<T>
		//is_promise_v<T>
		//判断是不是一个librf的promise_t类
		//
		//is_generator<T>
		//is_generator_v<T>
		//判断是不是一个librf的generator_t类
		//
		//has_state<T>
		//has_state_v<T>
		//判断是否具有_state的成员变量
		//
		//get_awaitor<T>(T&&t)
		//通过T获得其被co_await后的awaitor
		//
		//awaitor_traits<T>
		//获得一个awaitor的特征。
		//	type:awaitor的类型
		//	value_type:awaitor::await_resume()的返回值类型
		//
		//is_callable<T>
		//is_callable_v<T>
		//判断是不是一个可被调用的类型，如函数，仿函数，lambda等
		//
		//is_scheduler_task<T>
		//is_scheduler_task_v<T>
		//判断是不是可以被调度器调度的任务。调度器支持future和callable

		template<class _Ty>
		struct is_coroutine_handle : std::false_type {};
		template<class _PromiseT>
		struct is_coroutine_handle<coroutine_handle<_PromiseT>> : std::true_type {};
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
		struct is_state_pointer : std::false_type {};
		template<class _Ty>
		struct is_state_pointer<_Ty, std::void_t<std::enable_if_t<std::is_convertible_v<_Ty, state_base_t*>>>> : std::true_type {};
		template<class _Ty>
		struct is_state_pointer<counted_ptr<_Ty>> : is_state_pointer<_Ty> {};
		template<class _Ty>
		constexpr bool is_state_pointer_v = is_state_pointer<remove_cvref_t<_Ty>>::value;


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

		template<class _Ty, class = std::void_t<>>
		struct awaitor_traits{};

		template<class _Ty>
		struct awaitor_traits<_Ty, 
			std::void_t<decltype(get_awaitor(std::declval<_Ty>()))>
		>
		{
			using type = decltype(get_awaitor(std::declval<_Ty>()));
			using value_type = decltype(std::declval<type>().await_resume());
		};

		template<typename _Ty, class = std::void_t<>>
		struct is_callable : std::false_type{};
		template<typename _Ty>
		struct is_callable<_Ty, std::void_t<decltype(std::declval<_Ty>()())>> : std::true_type {};
		template<typename _Ty>
		constexpr bool is_callable_v = is_callable<_Ty>::value;

		template<class _Ty, class = std::void_t<>>
		struct is_iterator : std::false_type {};
		template<class _Ty>
		struct is_iterator
			<_Ty,
				std::void_t<
					decltype(++std::declval<_Ty>())
					, decltype(std::declval<_Ty>() != std::declval<_Ty>())
					, decltype(*std::declval<_Ty>())
				>
			>
			: std::true_type{};
		template<class _Ty>
		constexpr bool is_iterator_v = is_iterator<remove_cvref_t<_Ty>>::value;

		template<class _Ty, class = std::void_t<>>
		struct is_container : std::false_type {};
		template<class _Ty>
		struct is_container
			<_Ty,
				std::void_t<
					decltype(std::begin(std::declval<_Ty>()))
					, decltype(std::end(std::declval<_Ty>()))
				>
			>
			: is_iterator<decltype(std::begin(std::declval<_Ty>()))> {};
		template<class _Ty>
		constexpr bool is_container_v = is_container<remove_cvref_t<_Ty>>::value;
	}
}
