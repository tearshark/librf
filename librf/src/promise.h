#pragma once

#pragma push_macro("new")
#undef new

#ifndef DOXYGEN_SKIP_PROPERTY
namespace resumef
{
	/*
	Note: the awaiter object is part of coroutine state (as a temporary whose lifetime crosses a suspension point)
	and is destroyed before the co_await expression finishes.
	It can be used to maintain per-operation state as required by some async I/O APIs without resorting to additional heap allocations.
	*/

	struct suspend_on_initial
	{
		inline bool await_ready() noexcept
		{
			return false;
		}
		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		inline void await_suspend(coroutine_handle<_PromiseT> handler) noexcept
		{
			_PromiseT& promise = handler.promise();
			auto* _state = promise.get_state();
			_state->promise_initial_suspend(handler);
		}
		inline void await_resume() noexcept
		{
		}
	};

	struct suspend_on_final
	{
		inline bool await_ready() noexcept
		{
			return false;
		}
		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		inline void await_suspend(coroutine_handle<_PromiseT> handler) noexcept
		{
			_PromiseT& promise = handler.promise();
			auto _state = promise.ref_state();
			_state->promise_final_suspend(handler);
		}
		inline void await_resume() noexcept
		{
		}
	};

	template <typename _Ty>
	struct promise_impl_t
	{
		using value_type = _Ty;
		using state_type = state_t<value_type>;
		using promise_type = promise_t<value_type>;
		using future_type = future_t<value_type>;

		promise_impl_t() noexcept = default;
		promise_impl_t(promise_impl_t&& _Right) noexcept = default;
		promise_impl_t& operator = (promise_impl_t&& _Right) noexcept = default;
		promise_impl_t(const promise_impl_t&) = delete;
		promise_impl_t& operator = (const promise_impl_t&) = delete;

		auto get_state() noexcept->state_type*;
		// 如果去掉了调度器，则ref_state()实现为返回counted_ptr<>，以便于处理一些意外情况
		// auto ref_state() noexcept->counted_ptr<state_type>;
		auto ref_state() noexcept->state_type*;


		future_type get_return_object() noexcept
		{
			return { this->get_state() };
		}

		suspend_on_initial initial_suspend() noexcept
		{
			return {};
		}

		suspend_on_final final_suspend() noexcept
		{
			return {};
		}

		template <typename _Uty>
		_Uty&& await_transform(_Uty&& _Whatever) noexcept
		{
			if constexpr (traits::has_state_v<_Uty>)
			{
				_Whatever._state->set_scheduler(get_state()->get_scheduler());
			}

			return std::forward<_Uty>(_Whatever);
		}

		void unhandled_exception()		//If the coroutine ends with an uncaught exception, it performs the following: 
		{
			this->ref_state()->set_exception(std::current_exception());
		}

		void cancellation_requested() noexcept
		{

		}

/*
		using _Alloc_char = std::allocator<char>;
		void* operator new(size_t _Size);
		void operator delete(void* _Ptr, size_t _Size);
*/
#if !RESUMEF_INLINE_STATE
	private:
		counted_ptr<state_type> _state = state_future_t::_Alloc_state<state_type>(false);
#endif
	};

	template<class _Ty>
	struct promise_t final : public promise_impl_t<_Ty>
	{
		using typename promise_impl_t<_Ty>::value_type;
		using promise_impl_t<_Ty>::get_return_object;

		template<class U>
		void return_value(U&& val)	//co_return val
		{
			this->ref_state()->set_value(std::forward<U>(val));
		}

		template<class U>
		suspend_always yield_value(U&& val)
		{
			this->ref_state()->promise_yield_value(this, std::forward<U>(val));
			return {};
		}
	};

	template<class _Ty>
	struct promise_t<_Ty&> final : public promise_impl_t<_Ty&>
	{
		using typename promise_impl_t<_Ty&>::value_type;
		using promise_impl_t<_Ty&>::get_return_object;

		void return_value(_Ty& val)	//co_return val
		{
			this->ref_state()->set_value(val);
		}

		suspend_always yield_value(_Ty& val)
		{
			this->ref_state()->promise_yield_value(this, val);
			return {};
		}
	};

	template<>
	struct promise_t<void> final : public promise_impl_t<void>
	{
		using promise_impl_t<void>::get_return_object;

		void return_void()			//co_return;
		{
			this->ref_state()->set_value();
		}

		suspend_always yield_value()
		{
			this->ref_state()->promise_yield_value(this);
			return {};
		}
	};
#endif	//DOXYGEN_SKIP_PROPERTY
}

#pragma pop_macro("new")
