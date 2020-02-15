
#pragma once

#include "def.h"
#include "spinlock.h"
#include "counted_ptr.h"
#include <iostream>

namespace resumef
{
	struct state_base_t : public counted_t<state_base_t>
	{
		typedef std::recursive_mutex lock_type;

	protected:
		mutable lock_type _mtx;
		scheduler_t* _scheduler = nullptr;
		coroutine_handle<> _initor;
		//可能来自协程里的promise产生的，则经过co_await操作后，_coro在初始时不会为nullptr。
		//也可能来自awaitable_t，如果
		//		一、经过co_await操作后，_coro在初始时不会为nullptr。
		//		二、没有co_await操作，直接加入到了调度器里，则_coro在初始时为nullptr。调度器需要特殊处理此种情况。
		coroutine_handle<> _coro;
		std::exception_ptr _exception;
		state_base_t* _parent = nullptr;
#if RESUMEF_DEBUG_COUNTER
		intptr_t _id;
#endif
		bool _is_awaitor;
	public:
		state_base_t(bool awaitor)
		{
#if RESUMEF_DEBUG_COUNTER
			_id = ++g_resumef_state_id;
#endif
			_is_awaitor = awaitor;
		}

		RF_API virtual ~state_base_t();
		virtual bool has_value() const = 0;

		bool is_ready() const
		{
			return _is_awaitor == false || has_value() || _exception != nullptr;
		}

		void resume()
		{
			coroutine_handle<> handler;

			scoped_lock<lock_type> __guard(_mtx);
			if (_initor != nullptr)
			{
				handler = _initor;
				_initor = nullptr;
				handler();
			}
			else if(_coro != nullptr)
			{
				handler = _coro;
				_coro = nullptr;
				handler();
			}
		}
		coroutine_handle<> get_handler() const
		{
			return _coro;
		}
		bool has_handler() const
		{
			return _initor != nullptr || _coro != nullptr;
		}

		void set_scheduler(scheduler_t* sch)
		{
			scoped_lock<lock_type> __guard(_mtx);
			_scheduler = sch;
		}

		void set_scheduler_handler(scheduler_t* sch, coroutine_handle<> handler)
		{
			scoped_lock<lock_type> __guard(_mtx);
			_scheduler = sch;

			assert(_coro == nullptr);
			_coro = handler;
		}

		scheduler_t* get_scheduler() const
		{
			return _parent ? _parent->get_scheduler() : _scheduler;
		}

		state_base_t * get_parent() const
		{
			return _parent;
		}
/*
		const state_base_t* root_state() const
		{
			return _parent ? _parent->root_state() : this;
		}
		state_base_t* root_state()
		{
			return _parent ? _parent->root_state() : this;
		}
*/

		void set_exception(std::exception_ptr e);

		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		void future_await_suspend(coroutine_handle<_PromiseT> handler);

		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		void promise_initial_suspend(coroutine_handle<_PromiseT> handler);
		void promise_await_resume();
		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		void promise_final_suspend(coroutine_handle<_PromiseT> handler);
	};

	template <typename _Ty>
	struct state_t : public state_base_t
	{
		using state_base_t::state_base_t;
		using state_base_t::lock_type;
		using value_type = _Ty;
	protected:
		std::optional<value_type> _value;
	public:
		virtual bool has_value() const override
		{
			scoped_lock<lock_type> __guard(this->_mtx);
			return _value.has_value();
		}

		bool future_await_ready()
		{
			scoped_lock<lock_type> __guard(this->_mtx);
			return _value.has_value();
		}

		auto future_await_resume() -> value_type;

		void set_value(value_type val);
	};

	template<>
	struct state_t<void> : public state_base_t
	{
		using state_base_t::state_base_t;
		using state_base_t::lock_type;
	protected:
		std::atomic<bool> _has_value{ false };
	public:
		virtual bool has_value() const override
		{
			return _has_value;
		}

		bool future_await_ready()
		{
			return _has_value;
		}

		void future_await_resume();

		void set_value();
	};
}

