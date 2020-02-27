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
		virtual void destroy_deallocate() = 0;
	public:
		virtual void resume() = 0;
		virtual bool has_handler() const = 0;
		virtual bool is_ready() const = 0;
		virtual bool switch_scheduler_await_suspend(scheduler_t* sch, coroutine_handle<> handler) = 0;

		void set_scheduler(scheduler_t* sch)
		{
			_scheduler = sch;
		}
		coroutine_handle<> get_handler() const
		{
			return _coro;
		}
	};
	
	struct state_generator_t : public state_base_t
	{
		state_generator_t(coroutine_handle<> handler)
		{
			_coro = handler;
		}
	private:
		virtual void destroy_deallocate() override;
	public:
		virtual void resume() override;
		virtual bool has_handler() const override;
		virtual bool is_ready() const override;
		virtual bool switch_scheduler_await_suspend(scheduler_t* sch, coroutine_handle<> handler) override;

		static state_generator_t * _Alloc_state(coroutine_handle<> handler)
		{
#if RESUMEF_DEBUG_COUNTER
			std::cout << "state_generator_t::alloc, size=" << sizeof(state_generator_t) << std::endl;
#endif
			return new state_generator_t(handler);
		}
	};

	struct state_future_t : public state_base_t
	{
		typedef std::recursive_mutex lock_type;
	protected:
		mutable lock_type _mtx;
		coroutine_handle<> _initor;
		state_future_t* _parent = nullptr;
#if RESUMEF_DEBUG_COUNTER
		intptr_t _id;
#endif
		std::exception_ptr _exception;
		uint32_t _alloc_size;
		bool _has_value = false;
		bool _is_awaitor;
		bool _is_initor = false;
	public:
		state_future_t()
		{
#if RESUMEF_DEBUG_COUNTER
			_id = ++g_resumef_state_id;
#endif
			_is_awaitor = false;
		}
		explicit state_future_t(bool awaitor)
		{
#if RESUMEF_DEBUG_COUNTER
			_id = ++g_resumef_state_id;
#endif
			_is_awaitor = awaitor;
		}

		virtual void destroy_deallocate() override;
		virtual void resume() override;
		virtual bool has_handler() const override;
		virtual bool is_ready() const override;
		virtual bool switch_scheduler_await_suspend(scheduler_t* sch, coroutine_handle<> handler) override;

		scheduler_t* get_scheduler() const
		{
			return _parent ? _parent->get_scheduler() : _scheduler;
		}

		state_base_t * get_parent() const
		{
			return _parent;
		}
		uint32_t get_alloc_size() const
		{
			return _alloc_size;
		}

		void set_exception(std::exception_ptr e);

		template<class _Exp>
		void throw_exception(_Exp e)
		{
			set_exception(std::make_exception_ptr(std::move(e)));
		}

		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		void future_await_suspend(coroutine_handle<_PromiseT> handler);
		bool future_await_ready()
		{
			scoped_lock<lock_type> __guard(this->_mtx);
			return _has_value;
		}

		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		void promise_initial_suspend(coroutine_handle<_PromiseT> handler);
		void promise_await_resume();
		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		void promise_final_suspend(coroutine_handle<_PromiseT> handler);
	};

	template <typename _Ty>
	struct state_t final : public state_future_t
	{
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
	private:
		union union_value_type
		{
			value_type _value;
			char _[1];

			union_value_type() {}
			~union_value_type() {}
		};
		union_value_type uv;

		~state_t()
		{
			if (_has_value)
				uv._value.~value_type();
		}
	public:
		auto future_await_resume() -> value_type;
		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		void promise_yield_value(_PromiseT* promise, value_type val);

		void set_value(value_type val);
	};

	template<>
	struct state_t<void> final : public state_future_t
	{
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
		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		void promise_yield_value(_PromiseT* promise);

		void set_value();
	};
}

