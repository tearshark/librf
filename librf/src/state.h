
#pragma once

#include "def.h"
#include "counted_ptr.h"
#include <iostream>

namespace resumef
{
	template <typename T = void>
	struct promise_t;

	struct state_base
	{
	protected:
		std::mutex	_mtx;		//for value, _exception
		RF_API void set_value_none_lock();
	private:
		void * _this_promise = nullptr;
		scheduler * _current_scheduler = nullptr;
		std::vector<counted_ptr<state_base>> _depend_states;
	public:
		coroutine_handle<> _coro;
		std::atomic<intptr_t> _count = 0;				// tracks reference count of state object
		std::exception_ptr _exception;

		bool _ready = false;
		bool _cancellation = false;
		bool _done = false;

		state_base()
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

		void reset_none_lock()
		{
			_coro = nullptr;
			_ready = false;
		}

		void cancel()
		{
			scoped_lock<std::mutex> __guard(_mtx);

			_cancellation = true;
			_coro = nullptr;
		}

		bool is_suspend() const
		{
			return !_coro && !(_done || _ready || _cancellation);
		}
		void resume()
		{
			if (_coro)
			{
#if RESUMEF_DEBUG_COUNTER
				{
					scoped_lock<std::mutex> __lock(g_resumef_cout_mutex);

					std::cout << "scheduler=" << current_scheduler()
						<< ",coro=" << _coro.address()
						<< ",this_promise=" << this_promise()
						<< ",parent_promise=" << parent_promise()
						<< ",thread=" << std::this_thread::get_id()
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

		promise_t<void> * parent_promise() const;
		//scheduler * parent_scheduler() const;

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

		//------------------------------------------------------------------------------------------
		//以下是通过future_t/promise_t, 与编译器生成的resumable function交互的接口
		void await_suspend(coroutine_handle<> resume_cb);
		void final_suspend()
		{
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
			scoped_lock<std::mutex> __guard(_mtx);

			_value = t;
			state_base::set_value_none_lock();
		}
		void set_value(value_type&& t)
		{
			scoped_lock<std::mutex> __guard(_mtx);

			_value = std::forward<value_type>(t);
			state_base::set_value_none_lock();
		}
		_Ty & get_value()
		{
			scoped_lock<std::mutex> __guard(_mtx);

			if (!_ready)
				throw future_exception{ future_error::not_ready };
			return _value;
		}
		void reset()
		{
			scoped_lock<std::mutex> __guard(_mtx);
			state_base::reset_none_lock();
			_value = value_type{};
		}

		promise_t<_Ty> * parent_promise() const
		{
			return reinterpret_cast<promise_t<_Ty> *>(state_base::parent_promise());
		}
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
			scoped_lock<std::mutex> __guard(_mtx);

			set_value_none_lock();
		}
		void get_value()
		{
			scoped_lock<std::mutex> __guard(_mtx);

			if (!_ready)
				throw future_exception{ future_error::not_ready };
		}
		void reset()
		{
			scoped_lock<std::mutex> __guard(_mtx);

			reset_none_lock();
		}

		promise_t<void> * parent_promise() const
		{
			return reinterpret_cast<promise_t<void> *>(state_base::parent_promise());
		}
	};

}

