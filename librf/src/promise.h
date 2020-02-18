#pragma once
#include "state.h"

namespace resumef
{
	struct suspend_on_initial;
	struct suspend_on_final;

	template <typename _Ty>
	struct promise_impl_t
	{
		using value_type = _Ty;
		using state_type = state_t<value_type>;
		using promise_type = promise_t<value_type>;
		using future_type = future_t<value_type>;

		counted_ptr<state_type> _state = make_counted<state_type>(false);
		char __xxx[16];

		promise_impl_t() {}
		promise_impl_t(promise_impl_t&& _Right) noexcept = default;
		promise_impl_t& operator = (promise_impl_t&& _Right) noexcept = default;
		promise_impl_t(const promise_impl_t&) = delete;
		promise_impl_t& operator = (const promise_impl_t&) = delete;

		suspend_on_initial initial_suspend() noexcept;
		suspend_on_final final_suspend() noexcept;
		void set_exception(std::exception_ptr e);
#ifdef __clang__
		void unhandled_exception();
#endif
		future_type get_return_object();
		void cancellation_requested();

		using _Alloc_char = std::allocator<char>;
		void* operator new(size_t _Size)
		{
			std::cout << "promise::new, size=" << _Size << std::endl;

			_Alloc_char _Al;
			size_t _State_size = _Align_size<state_type>();
			char* ptr = _Al.allocate(_Size + _State_size);
			return ptr;
		}

		void operator delete(void* _Ptr, size_t _Size)
		{
			_Alloc_char _Al;
			return _Al.deallocate(static_cast<char*>(_Ptr), _Size);
		}
	};

	template<class _Ty>
	struct promise_t : public promise_impl_t<_Ty>
	{
		using typename promise_impl_t<_Ty>::value_type;
		using promise_impl_t<_Ty>::_state;

		void return_value(value_type val);
		void yield_value(value_type val);
	};

	template<>
	struct promise_t<void> : public promise_impl_t<void>
	{
		using promise_impl_t<void>::_state;

		void return_void();
		void yield_value();
	};

}

