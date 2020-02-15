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

		virtual state_base_t * get_state() const = 0;

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

		counted_ptr<state_type> _state;

		task_t() = default;
		task_t(future_type && f)
			: _state(std::move(f._state))
		{
		}

		virtual state_base_t * get_state() const
		{
			return _state.get();
		}
	};

	//----------------------------------------------------------------------------------------------

	//ctx_task_t接受的是一个'函数对象'
	//这个'函数对象'被调用后，返回generator<_Ty>/future_t<_Ty>类型
	//然后'函数对象'作为异步执行的上下文状态保存起来
	template<class _Ctx>
	struct ctx_task_t : public task_base_t
	{
		using context_type = _Ctx;
		using future_type = typename std::remove_cvref<decltype(std::declval<_Ctx>()())>::type;
		using value_type = typename future_type::value_type;
		using state_type = state_t<value_type>;

		context_type	_context;
		counted_ptr<state_type> _state;

		ctx_task_t(context_type ctx)
			: _context(std::move(ctx))
		{
			_state = _context()._state;
		}

		virtual state_base_t* get_state() const
		{
			return _state.get();
		}
	};
}
