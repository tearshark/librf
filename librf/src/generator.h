/***
*generator
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*       Purpose: Library support of coroutines. generator class
*       http://open-std.org/JTC1/SC22/WG21/docs/papers/2015/p0057r0.pdf
*
*       [Public]
*
****/
#pragma once
#ifndef _EXPERIMENTAL_GENERATOR_
#define _EXPERIMENTAL_GENERATOR_
#ifndef RC_INVOKED

#ifndef _RESUMABLE_FUNCTIONS_SUPPORTED
#error <experimental/generator> requires /await compiler option
#endif /* _RESUMABLE_FUNCTIONS_SUPPORTED */

#include <experimental/resumable>

#pragma pack(push, _CRT_PACKING)
#pragma push_macro("new")
#undef new

_STD_BEGIN

namespace experimental {

	template <typename _Ty, typename promise_type>
	struct generator_iterator;

	template<typename promise_type>
	struct generator_iterator<void, promise_type>
	{
		typedef _STD input_iterator_tag iterator_category;
		typedef ptrdiff_t difference_type;

		coroutine_handle<promise_type> _Coro;

		generator_iterator(nullptr_t) : _Coro(nullptr)
		{
		}

		generator_iterator(coroutine_handle<promise_type> _CoroArg) : _Coro(_CoroArg)
		{
		}

		generator_iterator &operator++()
		{
			if (_Coro.done())
				_Coro = nullptr;
			else
				_Coro.resume();
			return *this;
		}

		generator_iterator operator++(int) = delete;
		// generator iterator current_value
		// is a reference to a temporary on the coroutine frame
		// implementing postincrement will require storing a copy
		// of the value in the iterator.
		//{
		//	auto _Result = *this;
		//	++(*this);
		//	return _Result;
		//}

		bool operator==(generator_iterator const &right_) const
		{
			return _Coro == right_._Coro;
		}

		bool operator!=(generator_iterator const &right_) const
		{
			return !(*this == right_);
		}
	};

	template <typename promise_type>
	struct generator_iterator<nullptr_t, promise_type> : public generator_iterator<void, promise_type>
	{
		generator_iterator(nullptr_t) : generator_iterator<void, promise_type>(nullptr)
		{
		}
		generator_iterator(coroutine_handle<promise_type> _CoroArg) : generator_iterator<void, promise_type>(_CoroArg)
		{
		}
	};

	template <typename _Ty, typename promise_type>
	struct generator_iterator : public generator_iterator<void, promise_type>
	{
		generator_iterator(nullptr_t) : generator_iterator<void, promise_type>(nullptr)
		{
		}
		generator_iterator(coroutine_handle<promise_type> _CoroArg) : generator_iterator<void, promise_type>(_CoroArg)
		{
		}

		_Ty const &operator*() const
		{
			return *this->_Coro.promise()._CurrentValue;
		}

		_Ty const *operator->() const
		{
			return _STD addressof(operator*());
		}
	};

	template <typename _Ty, typename _Alloc = allocator<char>>
	struct generator
	{
		struct promise_type
		{
			_Ty const *_CurrentValue;

			promise_type &get_return_object()
			{
				return *this;
			}

			bool initial_suspend()
			{
				return (true);
			}

			bool final_suspend()
			{
				return (true);
			}

			void yield_value(_Ty const &_Value)
			{
				_CurrentValue = _STD addressof(_Value);
			}

			template<class = _STD enable_if_t<!is_same_v<_Ty, void>, _Ty>>
			void return_value(_Ty const &_Value)
			{
				_CurrentValue = _STD addressof(_Value);
			}
			template<class = _STD enable_if_t<is_same_v<_Ty, void>, _Ty>>
			void return_value()
			{
				_CurrentValue = nullptr;
			}

			template <typename _Uty>
			_Uty && await_transform(_Uty &&_Whatever)
			{
				static_assert(_Always_false<_Uty>::value,
					"co_await is not supported in coroutines of type std::experiemental::generator");
				return _STD forward<_Uty>(_Whatever);
			}

			using _Alloc_traits = allocator_traits<_Alloc>;
			using _Alloc_of_char_type =
				typename _Alloc_traits::template rebind_alloc<char>;

			void *operator new(size_t _Size)
			{
				_Alloc_of_char_type _Al;
				return _Al.allocate(_Size);
			}

			void operator delete(void *_Ptr, size_t _Size)
			{
				_Alloc_of_char_type _Al;
				return _Al.deallocate(static_cast<char *>(_Ptr), _Size);
			}
		};

		typedef generator_iterator<_Ty, promise_type> iterator;

		iterator begin()
		{
			if (_Coro)
			{
				if (_Coro.done())
					return{ nullptr };

				_Coro.resume();
			}
			return { _Coro };
		}

		iterator end()
		{
			return{ nullptr };
		}

		explicit generator(promise_type &_Prom)
			: _Coro(coroutine_handle<promise_type>::from_promise(
				_Prom))
		{
		}

		generator() = default;

		generator(generator const &) = delete;

		generator &operator=(generator const &) = delete;

		generator(generator &&right_) : _Coro(right_._Coro)
		{
			right_._Coro = nullptr;
		}

		generator &operator=(generator &&right_)
		{
			if (&right_ != this) {
				_Coro = right_._Coro;
				right_._Coro = nullptr;
			}
			return *this;
		}

		~generator()
		{
			if (_Coro) {
				_Coro.destroy();
			}
		}

	private:
		coroutine_handle<promise_type> _Coro = nullptr;
	};

} // namespace experimental

_STD_END

#pragma pop_macro("new")
#pragma pack(pop)
#endif /* RC_INVOKED */
#endif /* _EXPERIMENTAL_GENERATOR_ */
