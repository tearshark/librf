/*
* Modify from <experimental/generator_t.h>
* Purpose: Library support of coroutines. generator_t class
* http://open-std.org/JTC1/SC22/WG21/docs/papers/2015/p0057r0.pdf
*/
#pragma once

#pragma pack(push,_CRT_PACKING)
#pragma push_macro("new")
#undef new

namespace resumef
{
	template <typename _Ty, typename promise_type>
	struct generator_iterator;

#ifndef DOXYGEN_SKIP_PROPERTY

	template<typename promise_type>
	struct generator_iterator<void, promise_type>
	{
		typedef std::input_iterator_tag iterator_category;
		typedef ptrdiff_t difference_type;

		coroutine_handle<promise_type> _Coro;

		generator_iterator(std::nullptr_t) : _Coro(nullptr)
		{
		}

		generator_iterator(coroutine_handle<promise_type> _CoroArg) : _Coro(_CoroArg)
		{
		}

		generator_iterator& operator++()
		{
			if (_Coro.done())
				_Coro = nullptr;
			else
				_Coro.resume();
			return *this;
		}

		void operator++(int)
		{
			// This postincrement operator meets the requirements of the Ranges TS
			// InputIterator concept, but not those of Standard C++ InputIterator.
			++* this;
		}

		bool operator==(generator_iterator const& right_) const
		{
			return _Coro == right_._Coro;
		}

		bool operator!=(generator_iterator const& right_) const
		{
			return !(*this == right_);
		}
	};

	template <typename promise_type>
	struct generator_iterator<std::nullptr_t, promise_type> : public generator_iterator<void, promise_type>
	{
		generator_iterator(std::nullptr_t) : generator_iterator<void, promise_type>(nullptr)
		{
		}
		generator_iterator(coroutine_handle<promise_type> _CoroArg) : generator_iterator<void, promise_type>(_CoroArg)
		{
		}
	};

	template <typename _Ty, typename promise_type>
	struct generator_iterator : public generator_iterator<void, promise_type>
	{
		using value_type = _Ty;
		using reference = _Ty const&;
		using pointer = _Ty const*;

		generator_iterator(std::nullptr_t) : generator_iterator<void, promise_type>(nullptr)
		{
		}
		generator_iterator(coroutine_handle<promise_type> _CoroArg) : generator_iterator<void, promise_type>(_CoroArg)
		{
		}

		reference operator*() const
		{
			return *this->_Coro.promise()._CurrentValue;
		}

		pointer operator->() const
		{
			return this->_Coro.promise()._CurrentValue;
		}
	};
#endif	//DOXYGEN_SKIP_PROPERTY

	/**
	 * @brief 专用于co_yield函数。
	 */
	template <typename _Ty, typename _Alloc>
	struct generator_t
	{
		using value_type = _Ty;
		using state_type = state_generator_t;

#ifndef DOXYGEN_SKIP_PROPERTY
		struct promise_type
		{
			using value_type = _Ty;
			using state_type = state_generator_t;
			using future_type = generator_t<value_type>;

			_Ty const* _CurrentValue;

			promise_type()
			{
				get_state()->set_initial_suspend(coroutine_handle<promise_type>::from_promise(*this));
			}
			promise_type(promise_type&& _Right) noexcept = default;
			promise_type& operator = (promise_type&& _Right) noexcept = default;
			promise_type(const promise_type&) = default;
			promise_type& operator = (const promise_type&) = default;

			generator_t get_return_object()
			{
				return generator_t{ *this };
			}

			suspend_always initial_suspend() noexcept
			{
				return {};
			}

			suspend_always final_suspend() noexcept
			{
				return {};
			}

			suspend_always yield_value(_Ty const& _Value) noexcept
			{
				_CurrentValue = std::addressof(_Value);
				return {};
			}

			//template<class = std::enable_if_t<!std::is_same_v<_Ty, void>, _Ty>>
			void return_value(_Ty const& _Value) noexcept
			{
				_CurrentValue = std::addressof(_Value);
			}
			//template<class = std::enable_if_t<std::is_same_v<_Ty, void>, _Ty>>
			void return_value() noexcept
			{
				_CurrentValue = nullptr;
			}

			void set_exception(std::exception_ptr e)
			{
				(void)e;
				//ref_state()->set_exception(std::move(e));
				std::terminate();
			}
#if defined(__cpp_impl_coroutine) || defined(__clang__) || defined(__GNUC__)
			void unhandled_exception()
			{
				//this->ref_state()->set_exception(std::current_exception());
				std::terminate();
			}
#endif

			template <typename _Uty>
			_Uty&& await_transform(_Uty&& _Whatever) noexcept
			{
				static_assert(std::is_same_v<_Uty, void>,
					"co_await is not supported in coroutines of type std::experiemental::generator_t");
				return std::forward<_Uty>(_Whatever);
			}

			state_type* get_state() noexcept
			{
#if RESUMEF_INLINE_STATE
				size_t _State_size = _Align_size<state_type>();
#if defined(__clang__)
				auto h = coroutine_handle<promise_type>::from_promise(*this);
				char* ptr = reinterpret_cast<char*>(h.address()) - _State_size;
				return reinterpret_cast<state_type*>(ptr);
#elif defined(_MSC_VER)
				char* ptr = reinterpret_cast<char*>(this) - _State_size;
				return reinterpret_cast<state_type*>(ptr);
#else
#error "Unknown compiler"
#endif
#else
				return _state.get();
#endif
			}
			//counted_ptr<state_type> ref_state() noexcept
			//{
			//	return { get_state() };
			//}
			state_type* ref_state() noexcept
			{
				return get_state();
			}

			using _Alloc_char = typename std::allocator_traits<_Alloc>::template rebind_alloc<char>;
			static_assert(std::is_same_v<char*, typename std::allocator_traits<_Alloc_char>::pointer>,
				"generator_t does not support allocators with fancy pointer types");
			static_assert(std::allocator_traits<_Alloc_char>::is_always_equal::value,
				"generator_t only supports stateless allocators");

			void* operator new(size_t _Size)
			{
				_Alloc_char _Al;
#if RESUMEF_INLINE_STATE
				size_t _State_size = _Align_size<state_type>();
				assert(_Size >= sizeof(uint32_t) && _Size < (std::numeric_limits<uint32_t>::max)() - sizeof(_State_size));

				char* ptr = _Al.allocate(_Size + _State_size);
				char* _Rptr = ptr + _State_size;
#if RESUMEF_DEBUG_COUNTER
				std::cout << "  generator_promise::new, alloc size=" << (_Size + _State_size) << std::endl;
				std::cout << "  generator_promise::new, alloc ptr=" << (void*)ptr << std::endl;
				std::cout << "  generator_promise::new, return ptr=" << (void*)_Rptr << std::endl;
#endif

				//在初始地址上构造state
				{
					state_type* st = state_type::_Construct(ptr);
					st->lock();
				}

				return _Rptr;
#else
				char* ptr = _Al.allocate(_Size);
#if RESUMEF_DEBUG_COUNTER
				std::cout << "  generator_promise::new, alloc size=" << _Size << std::endl;
				std::cout << "  generator_promise::new, alloc ptr=" << (void*)ptr << std::endl;
				std::cout << "  generator_promise::new, return ptr=" << (void*)ptr << std::endl;
#endif

				return ptr;
#endif
			}

			void operator delete(void* _Ptr, size_t _Size)
			{
#if RESUMEF_INLINE_STATE
				size_t _State_size = _Align_size<state_type>();
				assert(_Size >= sizeof(uint32_t) && _Size < (std::numeric_limits<uint32_t>::max)() - sizeof(_State_size));

				*reinterpret_cast<uint32_t*>(_Ptr) = static_cast<uint32_t>(_Size + _State_size);

				state_type* st = reinterpret_cast<state_type*>(static_cast<char*>(_Ptr) - _State_size);
				st->unlock();
#else
				_Alloc_char _Al;
				return _Al.deallocate(reinterpret_cast<char *>(_Ptr), _Size);
#endif
			}
#if !RESUMEF_INLINE_STATE
		private:
			counted_ptr<state_type> _state = state_generator_t::_Alloc_state();
#endif
		};
#endif	//DOXYGEN_SKIP_PROPERTY

		using iterator = generator_iterator<_Ty, promise_type>;

		iterator begin()
		{
			if (_Coro)
			{
				_Coro.resume();
				if (_Coro.done())
					return{ nullptr };
			}
			return { _Coro };
		}

		iterator end()
		{
			return{ nullptr };
		}

		explicit generator_t(promise_type& _Prom)
			: _Coro(coroutine_handle<promise_type>::from_promise(_Prom))
		{
		}

		generator_t() = default;

		generator_t(generator_t const&) = delete;
		generator_t& operator=(generator_t const&) = delete;

		generator_t(generator_t&& right_) noexcept
			: _Coro(right_._Coro)
		{
			right_._Coro = nullptr;
		}

		generator_t& operator=(generator_t&& right_) noexcept
		{
			if (this != std::addressof(right_)) {
				_Coro = right_._Coro;
				right_._Coro = nullptr;
			}
			return *this;
		}

		~generator_t()
		{
			if (_Coro) {
				_Coro.destroy();
			}
		}

		state_type* detach_state()
		{
			auto t = _Coro;
			_Coro = nullptr;
			return t.promise().get_state();
		}

	private:
		coroutine_handle<promise_type> _Coro = nullptr;
	};

} // namespace resumef

#pragma pop_macro("new")
#pragma pack(pop)
