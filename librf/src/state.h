
#pragma once

#include "def.h"
#include "spinlock.h"
#include "counted_ptr.h"
#include <iostream>

namespace resumef
{
	extern std::atomic<intptr_t> g_resumef_static_count;

	struct state_base_t : public counted_t<state_base_t>
	{
		typedef std::recursive_mutex lock_type;

	protected:
		mutable lock_type _mtx;
		scheduler_t* _scheduler = nullptr;
		//可能来自协程里的promise产生的，则经过co_await操作后，_coro在初始时不会为nullptr。
		//也可能来自awaitable_t，如果
		//		一、经过co_await操作后，_coro在初始时不会为nullptr。
		//		二、没有co_await操作，直接加入到了调度器里，则_coro在初始时为nullptr。调度器需要特殊处理此种情况。
		coroutine_handle<> _coro;
		std::exception_ptr _exception;
	public:
		intptr_t _id;
		state_base_t* _parent = nullptr;

		state_base_t()
		{
			_id = ++g_resumef_static_count;
		}

		RF_API virtual ~state_base_t();
		virtual bool has_value() const = 0;

		bool is_ready() const
		{
			return has_value() && _exception != nullptr;
		}

		void resume()
		{
			scoped_lock<lock_type> __guard(_mtx);

			auto handler = _coro;
			_coro = nullptr;
			handler();
		}

		void set_handler(coroutine_handle<> handler)
		{
			scoped_lock<lock_type> __guard(_mtx);

			assert(_coro == nullptr);
			_coro = handler;
		}
		coroutine_handle<> get_handler() const
		{
			return _coro;
		}

		void set_scheduler(scheduler_t* sch)
		{
			scoped_lock<lock_type> __guard(_mtx);
			_scheduler = sch;
		}
		scheduler_t* get_scheduler() const
		{
			return _scheduler;
		}

		void set_exception(std::exception_ptr e);

		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		void future_await_suspend(coroutine_handle<_PromiseT> handler);

		void promise_final_suspend();
	};

	template <typename _Ty>
	struct state_t : public state_base_t
	{
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

