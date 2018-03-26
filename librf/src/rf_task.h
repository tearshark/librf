#pragma once

#include "def.h"
#include "future.h"

namespace resumef
{
	struct task_base;

	struct task_base
	{
		RF_API task_base();
		RF_API virtual ~task_base();

		//如果返回true，则不会调用go_next()
		virtual bool is_suspend() = 0;
		//返回true，表示任务还未完成，后续还需要继续执行
		//否则，任务从调度器里删除
		virtual bool go_next(scheduler *) = 0;
		virtual void cancel() = 0;
		virtual void * get_id() = 0;
#if RESUMEF_ENABLE_MULT_SCHEDULER
		virtual void bind(scheduler *) = 0;
#endif
	};

	//----------------------------------------------------------------------------------------------

	template<class _Ty>
	struct task_t;

	//task_t接受的是一个experimental::generator<_Ty>类型，是调用一个支持异步的函数后返回的结果
	template<class _Ty>
	struct task_t<std::experimental::generator<_Ty> > : public task_base
	{
		typedef std::experimental::generator<_Ty> future_type;
		typedef typename future_type::iterator iterator_type;

		future_type		_future;
		iterator_type	_iterator;
		bool			_ready;

		task_t()
			: _iterator(nullptr)
			, _ready(false)
		{
		}

		task_t(future_type && f)
			: _future(std::forward<future_type>(f))
			, _iterator(nullptr)
			, _ready(false)
		{
		}
		virtual bool is_suspend() override
		{
			return false;
		}
		virtual bool go_next(scheduler *) override
		{
			if (!this->_ready)
			{
				this->_iterator = this->_future.begin();
				this->_ready = true;
			}

			if (this->_iterator != this->_future.end())
			{
				return (++this->_iterator) != this->_future.end();
			}

			return false;
		}
		virtual void cancel() override
		{
		}
		virtual void * get_id() override
		{
			return nullptr;
		}
#if RESUMEF_ENABLE_MULT_SCHEDULER
		virtual void bind(scheduler *) override
		{

		}
#endif
	};

	template<class _Ty>
	struct task_t<future_t<_Ty> > : public task_base
	{
		typedef future_t<_Ty> future_type;

		future_type		_future;

		task_t() = default;
		task_t(future_type && f)
			: _future(std::forward<future_type>(f))
		{
		}

		//如果返回true，则不会调用go_next()
		//
		//如果第一次运行，则state应该有:
		//			_coro != nullptr
		//			_ready == false
		//运行一次后，则state应该是：
		//			_coro == nullptr
		//			_ready == false
		//最后一次运行，则state应该是：
		//			_coro == nullptr
		//			_ready == true
		virtual bool is_suspend() override
		{
			auto * _state = _future._state.get();
			return _state->is_suspend();
		}

		//返回true，表示任务还未完成，后续还需要继续执行
		//否则，任务从调度器里删除
		virtual bool go_next(scheduler * schdler) override
		{
			auto * _state = _future._state.get();
			_state->resume();
			return !_state->ready() && !_state->done();
		}
		virtual void cancel() override
		{
			_future.cancel();
		}
		virtual void * get_id() override
		{
			return _future._state.get();
		}
#if RESUMEF_ENABLE_MULT_SCHEDULER
		virtual void bind(scheduler * schdler) override
		{
			auto * _state = _future._state.get();
			_state->current_scheduler(schdler);
		}
#endif
	};

	//----------------------------------------------------------------------------------------------

	//ctx_task_t接受的是一个'函数对象'
	//这个'函数对象'被调用后，返回generator<_Ty>/future_t<_Ty>类型
	//然后'函数对象'作为异步执行的上下文状态保存起来
	template<class _Ctx>
	struct ctx_task_t : public task_t<typename std::result_of<_Ctx()>::type>
	{
		typedef task_t<typename std::result_of<_Ctx()>::type> base_type;
		using base_type::_future;

		typedef _Ctx context_type;
		context_type	_context;

		ctx_task_t(context_type && ctx)
			: _context(std::forward<context_type>(ctx))
		{
			_future = std::move(_context());
		}
	};

}
