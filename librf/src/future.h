
#pragma once

#include "state.h"

namespace resumef
{
	template <typename T>
	struct future_impl_t
	{
		typedef T value_type;
		typedef promise_t<T> promise_type;
		typedef state_t<T> state_type;
		counted_ptr<state_type> _state;

		future_impl_t(const counted_ptr<state_type>& state) : _state(state)
		{
		}

		// movable, but not copyable
		future_impl_t() = default;
		future_impl_t(future_impl_t&& f) = default;
		future_impl_t & operator = (future_impl_t&& f) = default;
		future_impl_t(const future_impl_t&) = default;
		future_impl_t & operator = (const future_impl_t&) = default;

		//------------------------------------------------------------------------------------------
		//以下是与编译器生成的resumable function交互的接口

		bool await_ready()
		{
			return _state->ready();
		}
		void await_suspend(coroutine_handle<> resume_cb)
		{
			_state->await_suspend(resume_cb);
		}

		//return_value, return_void只能由派生类去实现

		//以上是与编译器生成的resumable function交互的接口
		//------------------------------------------------------------------------------------------

		//if ready, can get value
		bool ready()
		{
			return _state->ready();
		}
		auto & get_value()
		{
			return _state->get_value();
		}
		bool is_suspend() const
		{
			return _state->is_suspend();
		}
		bool cancellation_requested() const
		{
			return _state->cancellation_requested();
		}

		void set_exception(std::exception_ptr e_)
		{
			_state->set_exception(std::move(e_));
		}

		void cancel()
		{
			_state->cancel();
		}
	};

	template <typename T = void>
	struct future_t : public future_impl_t<T>
	{
		using state_type = typename future_impl_t<T>::state_type;
		using future_impl_t<T>::_state;

		future_t(const counted_ptr<state_type>& state)
			: future_impl_t<T>(state)
		{
		}

		// movable, but not copyable
		future_t(const future_t&) = default;
		future_t(future_t&& f) = default;
		future_t() = default;

		future_t & operator = (const future_t&) = default;
		future_t & operator = (future_t&& f) = default;

		//------------------------------------------------------------------------------------------
		//以下是与编译器生成的resumable function交互的接口

		T await_resume()
		{
			_state->rethrow_if_exception();
			return std::move(_state->get_value());
		}

		void return_value(const T& val)
		{
			_state->set_value(val);
		}
		void return_value(T&& val)
		{
			_state->set_value(std::forward<T>(val));
		}

		//以上是与编译器生成的resumable function交互的接口
		//------------------------------------------------------------------------------------------
	};

	template <>
	struct future_t<void> : public future_impl_t<void>
	{
		using future_impl_t<void>::_state;

		future_t(const counted_ptr<state_type>& state)
			: future_impl_t<void>(state)
		{
		}

		// movable, but not copyable
		future_t(const future_t&) = default;
		future_t(future_t&& f) = default;
		future_t() = default;

		future_t & operator = (const future_t&) = default;
		future_t & operator = (future_t&& f) = default;

		//------------------------------------------------------------------------------------------
		//以下是与编译器生成的resumable function交互的接口

		void await_resume()
		{
			_state->rethrow_if_exception();
			return _state->get_value();
		}

		void return_void()
		{
			_state->set_value();
		}

		//以上是与编译器生成的resumable function交互的接口
		//------------------------------------------------------------------------------------------
	};

	using future_vt = future_t<void>;

	template <typename T>
	struct promise_impl_t
	{
		typedef future_t<T> future_type;
		typedef state_t<T> state_type;

		counted_ptr<state_type> _state;

		// movable not copyable
		promise_impl_t()
			: _state(make_counted<state_type>())
		{
#if RESUMEF_ENABLE_MULT_SCHEDULER
			_state->this_promise(this);
#endif
		}
		promise_impl_t(promise_impl_t&& _Right) noexcept
			: _state(std::move(_Right._state))
		{
#if RESUMEF_ENABLE_MULT_SCHEDULER
			_state->this_promise(this);
#endif
		}
		promise_impl_t & operator = (promise_impl_t&& _Right) noexcept
		{
			if (this != _Right)
			{
				_state = std::move(_Right._state);
#if RESUMEF_ENABLE_MULT_SCHEDULER
				_state->this_promise(this);
#endif
			}
			return *this;
		}
		promise_impl_t(const promise_impl_t&) = delete;
		promise_impl_t & operator = (const promise_impl_t&) = delete;

		//这个函数，用于callback里，返回关联的future对象
		//callback里，不应该调用 get_return_object()
		future_type get_future()
		{
			return future_type(_state);
		}

		// Most functions don't need this but timers and reads from streams
		// cause multiple callbacks.
		future_type next_future()
		{
			return future_type(_state);
		}

		//------------------------------------------------------------------------------------------
		//以下是与编译器生成的resumable function交互的接口

		//如果在协程外启动一个resumable function，则：
		//	1、即将交给调度器调度
		//	2、手工获取结果（将来支持）
		//	无论哪种情况，都返回未准备好。则编译器调用await_suspend()获得继续运行的resume_cb
		//如果在协程内启动一个resumable function，则：
		//	1、即将交给调度器调度
		//	2、通过await启动另外一个子函数
		//	(1)情况下，无法区分是否已经拥有的resume_cb，可以特殊处理
		//	(2)情况下，返回准备好了，让编译器继续运行
		auto initial_suspend() noexcept
		{
			return std::experimental::suspend_never{};
		}

		//这在一个协程被销毁之时调用。
		//我们选择不挂起协程，只是通知state的对象，本协程已经准备好了删除了
		auto final_suspend() noexcept
		{
			_state->final_suspend();
			return std::experimental::suspend_never{};
		}

		//返回与之关联的future对象
		future_type get_return_object()
		{
			return future_type(_state);
		}

		void set_exception(std::exception_ptr e_)
		{
			_state->set_exception(std::move(e_));
		}

		bool cancellation_requested()
		{
			return _state->cancellation_requested();
		}
/*
		void unhandled_exception() 
		{
			std::terminate(); 
		}
*/

		//return_value/return_void 只能由派生类去实现

		//以上是与编译器生成的resumable function交互的接口
		//------------------------------------------------------------------------------------------
	};

		
	template <typename T>
	struct promise_t : public promise_impl_t<T>
	{
		typedef promise_t<T> promise_type;
		using promise_impl_t<T>::_state;

		//------------------------------------------------------------------------------------------
		//以下是与编译器生成的resumable function交互的接口

		void return_value(const T& val)
		{
			_state->set_value(val);
		}
		void return_value(T&& val)
		{
			_state->set_value(std::forward<T>(val));
		}
		void yield_value(const T& val)
		{
			_state->set_value(val);
		}
		void yield_value(T&& val)
		{
			_state->set_value(std::forward<T>(val));
		}

		//以上是与编译器生成的resumable function交互的接口
		//------------------------------------------------------------------------------------------

		//兼容std::promise<>用法
		void set_value(const T& val)
		{
			_state->set_value(val);
		}
		void set_value(T&& val)
		{
			_state->set_value(std::forward<T>(val));
		}
	};

	template <>
	struct promise_t<void> : public promise_impl_t<void>
	{
		typedef promise_t<void> promise_type;
		using promise_impl_t<void>::_state;

		//------------------------------------------------------------------------------------------
		//以下是与编译器生成的resumable function交互的接口

		void return_void()
		{
			_state->set_value();
		}

		//以上是与编译器生成的resumable function交互的接口
		//------------------------------------------------------------------------------------------

		//兼容std::promise<>用法
		void set_value()
		{
			_state->set_value();
		}
	};

	using promise_vt = promise_t<void>;

	/*
	template <typename T = void>
	struct awaitable_t
	{
		typedef state_t<T> state_type;
		counted_ptr<state_type> _state;

		// movable not copyable
		awaitable_t()
			: _state(make_counted<state_type>())
		{
		}

		// movable, but not copyable
		awaitable_t(const awaitable_t&) = delete;
		awaitable_t(awaitable_t&& f) = default;

		awaitable_t & operator = (const awaitable_t&) = delete;
		awaitable_t & operator = (awaitable_t&& f) = default;

		//------------------------------------------------------------------------------------------
		//以下是与编译器生成的resumable function交互的接口

		bool await_ready()
		{
			return _state->ready();
		}
		void await_suspend(coroutine_handle<> resume_cb)
		{
			_state->await_suspend(resume_cb);
		}
		T await_resume()
		{
			_state->rethrow_if_exception();
			return _state->get_value();
		}

		//以上是与编译器生成的resumable function交互的接口
		//------------------------------------------------------------------------------------------
	};

	using awaitable_vt = awaitable_t<void>;
	*/

#if RESUMEF_ENABLE_MULT_SCHEDULER
	inline promise_t<void> * state_base::parent_promise() const
	{
		if (_coro) return _coro_promise_ptr__<promise_t<void>>(_coro.address());
		return nullptr;
	}

	inline scheduler * state_base::parent_scheduler() const
	{
		auto promise_ = parent_promise();
		if (promise_)
			return promise_->_state->current_scheduler();
		return nullptr;
	}
#endif

}

