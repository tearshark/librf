
#pragma once

#include "def.h"
#include "spinlock.h"
#include "counted_ptr.h"
#include <iostream>

namespace resumef
{
	template <typename T = void>
	struct promise_t;

	struct state_base
	{
	protected:
		//typedef spinlock lock_type;
		typedef std::recursive_mutex lock_type;

		lock_type	_mtx;		//for value, _exception
		RF_API void set_value_none_lock();
#if RESUMEF_ENABLE_MULT_SCHEDULER
	private:
		std::atomic<void *> _this_promise = nullptr;
		scheduler * _current_scheduler = nullptr;
		std::vector<counted_ptr<state_base>> _depend_states;
#endif
	protected:
		coroutine_handle<> _coro;
		std::atomic<intptr_t> _count{0};				// tracks reference count of state object
		std::exception_ptr _exception;

		std::atomic<bool> _ready = false;
		std::atomic<bool> _cancellation = false;
		std::atomic<bool> _done = false;

	public:
		state_base() noexcept
		{
#if RESUMEF_DEBUG_COUNTER
			++g_resumef_state_count;
#endif
		}
		//某个地方的代码，发生过截断，导致了内存泄漏。还是加上虚析构函数吧。
		virtual ~state_base();
		state_base(state_base&&) = delete;
		state_base(const state_base&) = delete;
		state_base & operator = (state_base&&) = delete;
		state_base & operator = (const state_base&) = delete;

		RF_API void set_exception(std::exception_ptr && e_);
		template<class _Exp>
		void throw_exception(const _Exp & e_)
		{
			set_exception(std::make_exception_ptr(e_));
		}

		void rethrow_if_exception()
		{
			if (_exception)
				std::rethrow_exception(_exception);
		}

		bool ready() const
		{
			return _ready;
		}
		bool done() const
		{
			return _done;
		}

		void reset_none_lock()
		{
			scoped_lock<lock_type> __guard(_mtx);

			_coro = nullptr;
			_ready = false;
		}

		void cancel()
		{
			scoped_lock<lock_type> __guard(_mtx);

			_cancellation = true;
			_coro = nullptr;
		}

		bool is_suspend() const
		{
			return !_coro && !(_done || _ready || _cancellation);
		}
		void resume()
		{
			scoped_lock<lock_type> __guard(_mtx);

			if (_coro)
			{
#if RESUMEF_DEBUG_COUNTER
				{
					scoped_lock<std::mutex> __lock(g_resumef_cout_mutex);

					std::cout 
						<< "coro=" << _coro.address()
						<< ",thread=" << std::this_thread::get_id()
#if RESUMEF_ENABLE_MULT_SCHEDULER

						<< ",scheduler=" << current_scheduler()
						<< ",this_promise=" << this_promise()
						<< ",parent_promise=" << parent_promise()
#endif
						<< std::endl;
				}
#endif
				auto coro = _coro;
				_coro = nullptr;

				coro();
			}
		}

		state_base* lock()
		{
			++_count;
			return this;
		}

		void unlock()
		{
			if (--_count == 0)
				delete this;
		}

#if RESUMEF_ENABLE_MULT_SCHEDULER
		promise_t<void> * parent_promise() const;
		scheduler * parent_scheduler() const;

		void * this_promise() const
		{
			return _this_promise;
		}
		void this_promise(void * promise_)
		{
			_this_promise = promise_;
		}

		scheduler * current_scheduler() const
		{
			return _current_scheduler;
		}
		void current_scheduler(scheduler * sch_);
#endif

		//------------------------------------------------------------------------------------------
		//以下是通过future_t/promise_t, 与编译器生成的resumable function交互的接口
		void await_suspend(coroutine_handle<> resume_cb);
		void final_suspend()
		{
			scoped_lock<lock_type> __guard(_mtx);
			_done = true;
		}
		//以上是通过future_t/promise_t, 与编译器生成的resumable function交互的接口
		//------------------------------------------------------------------------------------------
	};

	template <typename _Ty>
	struct state_t : public state_base
	{
	public:
		typedef _Ty value_type;
		value_type _value;

		state_t<_Ty>* lock()
		{
			++_count;
			return this;
		}

		void set_value(const value_type& t)
		{
			scoped_lock<lock_type> __guard(_mtx);

			_value = t;
			state_base::set_value_none_lock();
		}
		void set_value(value_type&& t)
		{
			scoped_lock<lock_type> __guard(_mtx);

			_value = std::forward<value_type>(t);
			state_base::set_value_none_lock();
		}

		value_type & set_value__()
		{
			scoped_lock<lock_type> __guard(_mtx);
			state_base::set_value_none_lock();

			return _value;
		}
		
		value_type & get_value()
		{
			scoped_lock<lock_type> __guard(_mtx);

			if (!_ready)
				throw future_exception{ error_code::not_ready };
			return _value;
		}

		void reset()
		{
			scoped_lock<lock_type> __guard(_mtx);
			state_base::reset_none_lock();
			_value = value_type{};
		}

#if RESUMEF_ENABLE_MULT_SCHEDULER
		promise_t<_Ty> * parent_promise() const
		{
			return reinterpret_cast<promise_t<_Ty> *>(state_base::parent_promise());
		}
#endif
	};

	template<>
	struct state_t<void> : public state_base
	{
		state_t<void>* lock()
		{
			++_count;
			return this;
		}

		void set_value()
		{
			scoped_lock<lock_type> __guard(_mtx);

			set_value_none_lock();
		}
		void get_value()
		{
			scoped_lock<lock_type> __guard(_mtx);

			if (!_ready)
				throw future_exception{ error_code::not_ready };
		}
		void reset()
		{
			scoped_lock<lock_type> __guard(_mtx);

			reset_none_lock();
		}

#if RESUMEF_ENABLE_MULT_SCHEDULER
		promise_t<void> * parent_promise() const
		{
			return reinterpret_cast<promise_t<void> *>(state_base::parent_promise());
		}
#endif
	};

}

