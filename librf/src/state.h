#pragma once

RESUMEF_NS
{
	struct state_base_t
	{
		using _Alloc_char = std::allocator<char>;
	private:
		std::atomic<intptr_t> _count{0};
	public:
		void lock()
		{
			++_count;
		}
		void unlock()
		{
			if (--_count == 0)
			{
				destroy_deallocate();
			}
		}
	protected:
		scheduler_t* _scheduler = nullptr;
		//可能来自协程里的promise产生的，则经过co_await操作后，_coro在初始时不会为nullptr。
		//也可能来自awaitable_t，如果
		//		一、经过co_await操作后，_coro在初始时不会为nullptr。
		//		二、没有co_await操作，直接加入到了调度器里，则_coro在初始时为nullptr。调度器需要特殊处理此种情况。
		coroutine_handle<> _coro;

		virtual ~state_base_t();
	private:
		virtual void destroy_deallocate();
	public:
		virtual void resume() = 0;
		virtual bool has_handler() const  noexcept = 0;
		virtual state_base_t* get_parent() const noexcept;

		void set_scheduler(scheduler_t* sch)
		{
			_scheduler = sch;
		}
		coroutine_handle<> get_handler() const
		{
			return _coro;
		}

		state_base_t* get_root() const noexcept
		{
			state_base_t* root = const_cast<state_base_t*>(this);
			state_base_t* next = root->get_parent();
			while (next != nullptr)
			{
				root = next;
				next = next->get_parent();
			}
			return root;
		}
		scheduler_t* get_scheduler() const
		{
			return get_root()->_scheduler;
		}
	};
	
	struct state_generator_t : public state_base_t
	{
	private:
		virtual void destroy_deallocate() override;
	public:
		virtual void resume() override;
		virtual bool has_handler() const  noexcept override;

		bool switch_scheduler_await_suspend(scheduler_t* sch);

		void set_initial_suspend(coroutine_handle<> handler)
		{
			_coro = handler;
		}

		static state_generator_t * _Alloc_state()
		{
			_Alloc_char _Al;
			size_t _Size = sizeof(state_generator_t);
#if RESUMEF_DEBUG_COUNTER
			std::cout << "state_generator_t::alloc, size=" << sizeof(state_generator_t) << std::endl;
#endif
			char* _Ptr = _Al.allocate(_Size);
			return new(_Ptr) state_generator_t();
		}
	};

	struct state_future_t : public state_base_t
	{
		enum struct initor_type : uint8_t
		{
			None,
			Initial,
			Final
		};
		enum struct result_type : uint8_t
		{
			None,
			Value,
			Exception,
		};

		//typedef std::recursive_mutex lock_type;
		typedef spinlock lock_type;
	protected:
		mutable lock_type _mtx;
		coroutine_handle<> _initor;
		state_future_t* _parent = nullptr;
#if RESUMEF_DEBUG_COUNTER
		intptr_t _id;
#endif
		uint32_t _alloc_size = 0;
		//注意：_has_value对齐到 4 Byte上，后面必须紧跟 _is_future变量。两者能组合成一个uint16_t数据。
		std::atomic<result_type> _has_value{ result_type::None };
		bool _is_future;
		initor_type _is_initor = initor_type::None;
		static_assert(sizeof(std::atomic<result_type>) == 1);
		static_assert(alignof(std::atomic<result_type>) == 1);
		static_assert(sizeof(bool) == 1);
		static_assert(alignof(bool) == 1);
		static_assert(sizeof(std::atomic<initor_type>) == 1);
		static_assert(alignof(std::atomic<initor_type>) == 1);
	public:
		state_future_t()
		{
#if RESUMEF_DEBUG_COUNTER
			_id = ++g_resumef_state_id;
#endif
			_is_future = true;
		}
		explicit state_future_t(bool awaitor)
		{
#if RESUMEF_DEBUG_COUNTER
			_id = ++g_resumef_state_id;
#endif
			_is_future = !awaitor;
		}

		virtual void destroy_deallocate() override;
		virtual void resume() override;
		virtual bool has_handler() const  noexcept override;
		virtual state_base_t* get_parent() const noexcept override;

		inline bool is_ready() const noexcept
		{
			//msvc认为是constexpr表达式(不写还给警告)，然而，clang不这么认为。
			//放弃constexpr，反正合格的编译器都会优化掉这个if判断的。
			if 
#ifndef __clang__
				constexpr
#endif
				(_offset_of(state_future_t, _is_future) - _offset_of(state_future_t, _has_value) == 1)
				return 0 != reinterpret_cast<const std::atomic<uint16_t> &>(_has_value).load(std::memory_order_acquire);
			else
				return _has_value.load(std::memory_order_acquire) != result_type::None || _is_future;
		}
		inline bool has_handler_skip_lock() const noexcept
		{
			return (bool)_coro || _is_initor != initor_type::None;
		}

		inline uint32_t get_alloc_size() const noexcept
		{
			return _alloc_size;
		}

		inline bool future_await_ready() noexcept
		{
			//scoped_lock<lock_type> __guard(this->_mtx);
			return _has_value.load(std::memory_order_acquire) != result_type::None;
		}
		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		void future_await_suspend(coroutine_handle<_PromiseT> handler);

		bool switch_scheduler_await_suspend(scheduler_t* sch);

		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		void promise_initial_suspend(coroutine_handle<_PromiseT> handler);
		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		void promise_final_suspend(coroutine_handle<_PromiseT> handler);

		template<class _Sty>
		static inline _Sty* _Alloc_state(bool awaitor)
		{
			_Alloc_char _Al;
			size_t _Size = sizeof(_Sty);
#if RESUMEF_DEBUG_COUNTER
			std::cout << "state_future_t::alloc, size=" << _Size << std::endl;
#endif
			char* _Ptr = _Al.allocate(_Size);
			return new(_Ptr) _Sty(awaitor);
		}
	};

	template <typename _Ty>
	struct state_t final : public state_future_t
	{
		friend state_future_t;

		using state_future_t::lock_type;
		using value_type = _Ty;

		explicit state_t(size_t alloc_size) :state_future_t()
		{
			_alloc_size = static_cast<uint32_t>(alloc_size);
		}
		explicit state_t(bool awaitor) :state_future_t(awaitor)
		{
			_alloc_size = sizeof(*this);
		}
	public:
		~state_t()
		{
			switch (_has_value.load(std::memory_order_acquire))
			{
			case result_type::Value:
				_value.~value_type();
				break;
			case result_type::Exception:
				_exception.~exception_ptr();
				break;
			default:
				break;
			}
		}

		auto future_await_resume() -> value_type;
		template<class _PromiseT, typename U, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		void promise_yield_value(_PromiseT* promise, U&& val);

		void set_exception(std::exception_ptr e);
		template<typename U>
		void set_value(U&& val);

		template<class _Exp>
		inline void throw_exception(_Exp e)
		{
			set_exception(std::make_exception_ptr(std::move(e)));
		}
	private:
		union
		{
			std::exception_ptr _exception;
			value_type _value;
		};

		template<typename U>
		void set_value_internal(U&& val);
		void set_exception_internal(std::exception_ptr e);
	};

	template <typename _Ty>
	struct state_t<_Ty&> final : public state_future_t
	{
		friend state_future_t;

		using state_future_t::lock_type;
		using value_type = _Ty;
		using reference_type = _Ty&;

		explicit state_t(size_t alloc_size) :state_future_t()
		{
			_alloc_size = static_cast<uint32_t>(alloc_size);
		}
		explicit state_t(bool awaitor) :state_future_t(awaitor)
		{
			_alloc_size = sizeof(*this);
		}
	public:
		~state_t()
		{
			if (_has_value.load(std::memory_order_acquire) == result_type::Exception)
				_exception.~exception_ptr();
		}

		auto future_await_resume()->reference_type;
		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		void promise_yield_value(_PromiseT* promise, reference_type val);

		void set_exception(std::exception_ptr e);
		void set_value(reference_type val);

		template<class _Exp>
		inline void throw_exception(_Exp e)
		{
			set_exception(std::make_exception_ptr(std::move(e)));
		}
	private:
		union
		{
			std::exception_ptr _exception;
			value_type* _value;
		};

		void set_value_internal(reference_type val);
		void set_exception_internal(std::exception_ptr e);
	};

	template<>
	struct state_t<void> final : public state_future_t
	{
		friend state_future_t;
		using state_future_t::lock_type;

		explicit state_t(size_t alloc_size) :state_future_t()
		{
			_alloc_size = static_cast<uint32_t>(alloc_size);
		}
		explicit state_t(bool awaitor) :state_future_t(awaitor)
		{
			_alloc_size = sizeof(*this);
		}
	public:
		void future_await_resume();
		template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
		void promise_yield_value(_PromiseT* promise);

		void set_exception(std::exception_ptr e);
		void set_value();

		template<class _Exp>
		inline void throw_exception(_Exp e)
		{
			set_exception(std::make_exception_ptr(std::move(e)));
		}
	private:
		std::exception_ptr _exception;
	};
}

