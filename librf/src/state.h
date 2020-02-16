
#pragma once

#include "def.h"
#include "spinlock.h"
#include "counted_ptr.h"
#include <iostream>

namespace resumef
{
	struct state_base_t : public counted_t<state_base_t>
	{
		RF_API virtual ~state_base_t();
	protected:
		scheduler_t* _scheduler = nullptr;
		//可能来自协程里的promise产生的，则经过co_await操作后，_coro在初始时不会为nullptr。
		//也可能来自awaitable_t，如果
		//		一、经过co_await操作后，_coro在初始时不会为nullptr。
		//		二、没有co_await操作，直接加入到了调度器里，则_coro在初始时为nullptr。调度器需要特殊处理此种情况。
		coroutine_handle<> _coro;
	public:
		virtual void resume() = 0;
		virtual bool has_handler() const = 0;
		virtual bool is_ready() const = 0;

		void set_scheduler(scheduler_t* sch)
		{
			_scheduler = sch;
		}
		coroutine_handle<> get_handler() const
		{
			return _coro;
		}
	};
	
	struct state_generator_t : public state_base_t
	{
	public:
		state_generator_t(coroutine_handle<> handler)
		{
			_coro = handler;
		}

		virtual void resume() override;
		virtual bool has_handler() const override;
		virtual bool is_ready() const override;
	};

	struct state_future_t : public state_base_t
	{
		typedef std::recursive_mutex lock_type;
	protected:
		mutable lock_type _mtx;
		coroutine_handle<> _initor;
		std::exception_ptr _exception;
		state_future_t* _parent = nullptr;
#if RESUMEF_DEBUG_COUNTER
		intptr_t _id;
#endif
		bool _is_awaitor;
	public:
		state_future_t(bool awaitor)
		{
#if RESUMEF_DEBUG_COUNTER
			_id = ++g_resumef_state_id;
#endif
			_is_awaitor = awaitor;
		}

		virtual void resume() override;
		virtual bool has_handler() const override;

		scheduler_t* get_scheduler() const
		{
			return _parent ? _parent->get_scheduler() : _scheduler;
		}

		state_base_t * get_parent() const
		{
			return _parent;
		}

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
	struct state_t : public state_future_t
	{
		using state_future_t::state_future_t;
		using state_future_t::lock_type;
		using value_type = _Ty;
	protected:
		std::optional<value_type> _value;
	public:
		virtual bool is_ready() const override
		{
			scoped_lock<lock_type> __guard(this->_mtx);
			return _is_awaitor == false || _value.has_value() || _exception != nullptr;
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
	struct state_t<void> : public state_future_t
	{
		using state_future_t::state_future_t;
		using state_future_t::lock_type;
	protected:
		std::atomic<bool> _has_value{ false };
	public:
		virtual bool is_ready() const override;

		bool future_await_ready()
		{
			return _has_value;
		}

		void future_await_resume();

		void set_value();
	};
}

