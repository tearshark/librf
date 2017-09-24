
#pragma once

#include "def.h"
#include <iostream>

namespace resumef
{
	struct state_base
	{
	protected:
		std::mutex	_mtx;		//for value, _exception
		RF_API void set_value_none_lock();
	public:
		std::experimental::coroutine_handle<> _coro;
		std::atomic<intptr_t> _count = 0;				// tracks reference count of state object
		std::exception_ptr _exception;

		bool _ready = false;
		bool _future_acquired = false;
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
			_future_acquired = false;
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
				auto coro = _coro;
				_coro = nullptr;
				//std::cout << "resume from " << coro.address() << " on thread " << std::this_thread::get_id() << std::endl;
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

		//------------------------------------------------------------------------------------------
		//以下是通过future_t/promise_t, 与编译器生成的resumable function交互的接口

		bool await_ready()
		{
			return _ready;
		}
		void await_suspend(std::experimental::coroutine_handle<> resume_cb)
		{
			_coro = resume_cb;
		}
		void await_resume()
		{
		}
		void final_suspend()
		{
			_done = true;
		}
		bool cancellation_requested() const
		{
			return _cancellation;
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
	};

	// counted_ptr is similar to shared_ptr but allows explicit control
	// 
	template <typename T>
	struct counted_ptr
	{
		counted_ptr() = default;
		counted_ptr(const counted_ptr& cp) : _p(cp._p)
		{
			_lock();
		}

		counted_ptr(T* p) : _p(p)
		{
			_lock();
		}

		counted_ptr(counted_ptr&& cp)
		{
			std::swap(_p, cp._p);
		}

		counted_ptr& operator=(const counted_ptr& cp)
		{
			if (&cp != this)
			{
				_unlock();
				_lock(cp._p);
			}
			return *this;
		}

		counted_ptr& operator=(counted_ptr&& cp)
		{
			if (&cp != this)
				std::swap(_p, cp._p);
			return *this;
		}

		~counted_ptr()
		{
			_unlock();
		}

		T* operator->() const
		{
			return _p;
		}

		T* get() const
		{
			return _p;
		}

		void reset()
		{
			_unlock();
		}
	protected:
		void _unlock()
		{
			if (_p != nullptr)
			{
				auto t = _p;
				_p = nullptr;
				t->unlock();
			}
		}
		void _lock(T* p)
		{
			if (p != nullptr)
				p->lock();
			_p = p;
		}
		void _lock()
		{
			if (_p != nullptr)
				_p->lock();
		}
		T* _p = nullptr;
	};

	template <typename T>
	counted_ptr<T> make_counted()
	{
		return new T{};
	}
}

