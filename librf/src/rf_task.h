#pragma once

RESUMEF_NS
{
	struct task_base_t;

	struct task_base_t
	{
		task_base_t() = default;
		virtual ~task_base_t();

		state_base_t* get_state() const noexcept
		{
			return _state.get();
		}
	protected:
		counted_ptr<state_base_t> _state;
	};

	//----------------------------------------------------------------------------------------------

	template<class _Ty, class = std::void_t<>>
	struct task_t;

	template<class _Ty>
	struct task_t<_Ty, std::void_t<is_future<std::remove_reference_t<_Ty>>>> : public task_base_t
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
}
