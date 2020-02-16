#pragma once

#include "def.h"
#include "future.h"
#include "promise.h"

namespace resumef
{
	struct task_base_t;

	struct task_base_t
	{
		RF_API task_base_t();
		RF_API virtual ~task_base_t();

		state_base_t* get_state() const
		{
			return _state.get();
		}
	protected:
		counted_ptr<state_base_t> _state;
	public:
		task_base_t* _next_node;
		task_base_t* _prev_node;
	};

	//----------------------------------------------------------------------------------------------

	template<class _Ty>
	struct task_t;

	template<class _Ty>
	struct task_t<future_t<_Ty>> : public task_base_t
	{
		using value_type = _Ty;
		using future_type = future_t<value_type>;
		using state_type = state_t<value_type>;

		task_t() = default;
		task_t(future_type && f)
		{
			initialize(std::forward<future_type>(f));
		}
	protected:
		void initialize(future_type&& f)
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
		task_t(future_type&& f)
		{
			initialize(std::forward<future_type>(f));
		}
	protected:
		void initialize(future_type&& f)
		{
			_state = new state_type(f.detach());
		}
	};

	//----------------------------------------------------------------------------------------------

	//ctx_task_t接受的是一个'函数对象'
	//这个'函数对象'被调用后，返回generator<_Ty>/future_t<_Ty>类型
	//然后'函数对象'作为异步执行的上下文状态保存起来
	template<class _Ctx>
	struct ctx_task_t : public task_t<typename std::remove_cvref<decltype(std::declval<_Ctx>()())>::type>
	{
		using context_type = _Ctx;

		context_type	_context;

		ctx_task_t(context_type ctx)
			: _context(std::move(ctx))
		{
			this->initialize(_context());
		}
	};
}
