#pragma once

#ifndef DOXYGEN_SKIP_PROPERTY
RESUMEF_NS
{
	/**
	 * @brief 协程任务的基类。
	 */
	struct task_base_t
	{
		task_base_t() = default;
		virtual ~task_base_t();

	protected:
		friend scheduler_t;
		counted_ptr<state_base_t> _state;
	};
#endif

	//----------------------------------------------------------------------------------------------

	/**
	 * @brief 协程任务类。
	 * @details 每启动一个新的协程，则对应一个协程任务类。\n
	 * 一方面，task_t<>用于标记协程是否执行完毕；\n
	 * 另一方面，对于通过函数对象(functor/lambda)启动的协程，有很大概率，此协程的内部变量，依赖此函数对象的生存期。\n
	 * tast_t<>的针对函数对象的特化版本，会持有此函数对象的拷贝，从而保证协程内部变量的生存期。从而减少外部使用协程函数对象的工作量。\n
	 * 如果不希望task_t<>持有此函数对象，则通过调用此函数对象来启动协程，即:\n
	 * go functor; \n
	 * 替换为\n
	 * go functor();
	 */
	template<class _Ty, class = std::void_t<>>
	struct task_t;

#ifndef DOXYGEN_SKIP_PROPERTY
	template<class _Ty>
	struct task_t<_Ty, std::void_t<traits::is_future<std::remove_reference_t<_Ty>>>> : public task_base_t
	{
		using future_type = std::remove_reference_t<_Ty>;
		using value_type = typename future_type::value_type;
		using state_type = state_t<value_type>;

		task_t() = default;
		task_t(future_type& f)
		{
			initialize(f);
		}
	protected:
		void initialize(future_type& f)
		{
			_state = f._state.get();
		}
	};

	template<class _Ty>
	struct task_t<generator_t<_Ty>> : public task_base_t
	{
		using value_type = _Ty;
		using future_type = generator_t<value_type>;
		using state_type = state_generator_t;

		task_t() = default;
		task_t(future_type& f)
		{
			initialize(f);
		}
	protected:
		void initialize(future_type& f)
		{
			_state = f.detach_state();
		}
	};

	//----------------------------------------------------------------------------------------------

	//ctx_task_t接受的是一个'函数对象'
	//这个'函数对象'被调用后，返回generator<_Ty>/future_t<_Ty>类型
	//然后'函数对象'作为异步执行的上下文状态保存起来
	template<class _Ctx>
	struct ctx_task_t : public task_t<remove_cvref_t<decltype(std::declval<_Ctx>()())>>
	{
		using context_type = _Ctx;

		context_type	_context;

		ctx_task_t(context_type ctx)
			: _context(std::move(ctx))
		{
			decltype(auto) f = _context();
			this->initialize(f);
		}
	};
#endif	//DOXYGEN_SKIP_PROPERTY
}
