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

	template<typename promise_type>
	struct generator_iterator<void, promise_type>
	{
		typedef std::input_iterator_tag iterator_category;
		typedef ptrdiff_t difference_type;

		coroutine_handle<promise_type> _Coro;

		generator_iterator(nullptr_t) : _Coro(nullptr)
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
		using value_type = _Ty;
		using reference = _Ty const&;
		using pointer = _Ty const*;

		generator_iterator(nullptr_t) : generator_iterator<void, promise_type>(nullptr)
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

	template <typename _Ty, typename _Alloc>
	struct generator_t
	{
		using value_type = _Ty;
		using state_type = state_generator_t;

		struct promise_type
		{
			using value_type = _Ty;
			using state_type = state_generator_t;
			using future_type = generator_t<value_type>;

			_Ty const* _CurrentValue;

			promise_type()
			{
				state_type* st = get_state();
				new(st) state_type(coroutine_handle<promise_type>::from_promise(*this));
				st->lock();
			}
			promise_type(promise_type&& _Right) noexcept = default;
			promise_type& operator = (promise_type&& _Right) noexcept = default;
			promise_type(const promise_type&) = delete;
			promise_type& operator = (const promise_type&) = delete;

			promise_type& get_return_object()
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

			void yield_value(_Ty const& _Value)
			{
				_CurrentValue = std::addressof(_Value);
			}

			template<class = std::enable_if_t<!std::is_same_v<_Ty, void>, _Ty>>
			void return_value(_Ty const& _Value)
			{
				_CurrentValue = std::addressof(_Value);
			}
			template<class = std::enable_if_t<std::is_same_v<_Ty, void>, _Ty>>
			void return_value()
			{
				_CurrentValue = nullptr;
			}

			template <typename _Uty>
			_Uty&& await_transform(_Uty&& _Whatever)
			{
				static_assert(std::is_same_v<_Uty, void>,
					"co_await is not supported in coroutines of type std::experiemental::generator_t");
				return std::forward<_Uty>(_Whatever);
			}

			state_type* get_state()
			{
				size_t _State_size = _Align_size<state_type>();
				char* ptr = reinterpret_cast<char*>(this) - _State_size;
				return reinterpret_cast<state_type*>(ptr);
			}

			using _Alloc_char = typename std::allocator_traits<_Alloc>::template rebind_alloc<char>;
			static_assert(std::is_same_v<char*, typename std::allocator_traits<_Alloc_char>::pointer>,
				"generator_t does not support allocators with fancy pointer types");
			static_assert(std::allocator_traits<_Alloc_char>::is_always_equal::value,
				"generator_t only supports stateless allocators");

			void* operator new(size_t _Size)
			{
				size_t _State_size = _Align_size<state_type>();
				assert(_Size >= sizeof(uint32_t) && _Size < std::numeric_limits<uint32_t>::max() - sizeof(_State_size));
#if RESUMEF_DEBUG_COUNTER
				std::cout << "generator_promise::new, size=" << (_Size + _State_size) << std::endl;
#endif

				_Alloc_char _Al;
				char* ptr = _Al.allocate(_Size + _State_size);
				return ptr + _State_size;
			}

			void operator delete(void* _Ptr, size_t _Size)
			{
				size_t _State_size = _Align_size<state_type>();
				assert(_Size >= sizeof(uint32_t) && _Size < std::numeric_limits<uint32_t>::max() - sizeof(_State_size));

				*reinterpret_cast<uint32_t*>(_Ptr) = static_cast<uint32_t>(_Size + _State_size);

				_Alloc_char _Al;
				state_type* st = reinterpret_cast<state_type*>(static_cast<char*>(_Ptr) - _State_size);
				st->unlock();
			}
		};

		typedef generator_iterator<_Ty, promise_type> iterator;

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
