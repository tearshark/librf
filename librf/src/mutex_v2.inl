#pragma once

RESUMEF_NS
{
	namespace detail
	{
		struct state_mutex_t : public state_base_t
		{
			virtual void resume() override;
			virtual bool has_handler() const  noexcept override;
			virtual state_base_t* get_parent() const noexcept override;

			bool on_notify();

			inline scheduler_t* get_scheduler() const noexcept
			{
				return _scheduler;
			}

			inline void on_await_suspend(coroutine_handle<> handler, scheduler_t* sch, state_base_t* root) noexcept
			{
				this->_scheduler = sch;
				this->_coro = handler;
				this->_root = root;
			}

		private:
			state_base_t* _root;
			//friend mutex_v2::mutex_t;
		};

		//做成递归锁?
		struct mutex_v2_impl : public std::enable_shared_from_this<mutex_v2_impl>
		{
			mutex_v2_impl() {}

			inline void* owner() const noexcept
			{
				return _owner.load(std::memory_order_relaxed);
			}

			bool try_lock(void* sch);					//内部加锁
			bool unlock(void* sch);						//内部加锁
			void lock_until_succeed(void* sch);			//内部加锁
		public:
			static constexpr bool USE_SPINLOCK = true;

			using lock_type = std::conditional_t<USE_SPINLOCK, spinlock, std::recursive_mutex>;
			using state_mutex_ptr = counted_ptr<state_mutex_t>;
			using wait_queue_type = std::list<state_mutex_ptr>;

			bool try_lock_lockless(void* sch) noexcept;			//内部不加锁，加锁由外部来进行
			void add_wait_list_lockless(state_mutex_t* state);	//内部不加锁，加锁由外部来进行

			lock_type _lock;									//保证访问本对象是线程安全的
		private:
			std::atomic<void*> _owner = nullptr;				//锁标记
			std::atomic<intptr_t> _counter = 0;					//递归锁的次数
			wait_queue_type _wait_awakes;						//等待队列

			// No copying/moving
			mutex_v2_impl(const mutex_v2_impl&) = delete;
			mutex_v2_impl(mutex_v2_impl&&) = delete;
			mutex_v2_impl& operator=(const mutex_v2_impl&) = delete;
			mutex_v2_impl& operator=(mutex_v2_impl&&) = delete;
		};
	}

	inline namespace mutex_v2
	{
		struct scoped_lock_mutex_t
		{
			typedef std::shared_ptr<detail::mutex_v2_impl> mutex_impl_ptr;

			//此函数，应该在try_lock()获得锁后使用
			//或者在协程里，由awaiter使用
			scoped_lock_mutex_t(std::adopt_lock_t, mutex_impl_ptr mtx, void* sch)
				: _mutex(std::move(mtx))
				, _owner(sch)
			{}

			//此函数，适合在非协程里使用
			scoped_lock_mutex_t(mutex_impl_ptr mtx, void* sch)
				: _mutex(std::move(mtx))
				, _owner(sch)
			{
				if (_mutex != nullptr)
					_mutex->lock_until_succeed(sch);
			}


			scoped_lock_mutex_t(std::adopt_lock_t, const mutex_t& mtx, void* sch)
				: scoped_lock_mutex_t(std::adopt_lock, mtx._mutex, sch)
			{}
			scoped_lock_mutex_t(const mutex_t& mtx, void* sch)
				: scoped_lock_mutex_t(mtx._mutex, sch)
			{}

			~scoped_lock_mutex_t()
			{
				if (_mutex != nullptr)
					_mutex->unlock(_owner);
			}

			inline void unlock() noexcept
			{
				if (_mutex != nullptr)
				{
					_mutex->unlock(_owner);
					_mutex = nullptr;
				}
			}

			scoped_lock_mutex_t(const scoped_lock_mutex_t&) = delete;
			scoped_lock_mutex_t& operator = (const scoped_lock_mutex_t&) = delete;
			scoped_lock_mutex_t(scoped_lock_mutex_t&&) = default;
			scoped_lock_mutex_t& operator = (scoped_lock_mutex_t&&) = default;
		private:
			mutex_impl_ptr _mutex;
			void* _owner;
		};


		struct [[nodiscard]] mutex_t::awaiter
		{
			awaiter(detail::mutex_v2_impl* mtx) noexcept
				: _mutex(mtx)
			{
				assert(_mutex != nullptr);
			}

			~awaiter() noexcept(false)
			{
				assert(_mutex == nullptr);
				if (_mutex != nullptr)
				{
					throw lock_exception(error_code::not_await_lock);
				}
			}

			bool await_ready() noexcept
			{
				return false;
			}

			template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
			bool await_suspend(coroutine_handle<_PromiseT> handler)
			{
				_PromiseT& promise = handler.promise();
				auto* parent = promise.get_state();
				_root = parent->get_root();

				scoped_lock<detail::mutex_v2_impl::lock_type> lock_(_mutex->_lock);
				if (_mutex->try_lock_lockless(_root))
					return false;

				_state = new detail::state_mutex_t();
				_state->on_await_suspend(handler, parent->get_scheduler(), _root);

				_mutex->add_wait_list_lockless(_state.get());

				return true;
			}

			scoped_lock_mutex_t await_resume() noexcept
			{
				mutex_impl_ptr mtx = _root ? _mutex->shared_from_this() : nullptr;
				_mutex = nullptr;

				return { std::adopt_lock, mtx, _root };
			}
		protected:
			detail::mutex_v2_impl* _mutex;
			counted_ptr<detail::state_mutex_t> _state;
			state_base_t* _root = nullptr;
		};

		inline mutex_t::awaiter mutex_t::operator co_await() const noexcept
		{
			return { _mutex.get() };
		}

		inline mutex_t::awaiter mutex_t::lock() const noexcept
		{
			return { _mutex.get() };
		}

		inline scoped_lock_mutex_t mutex_t::lock(void* unique_address) const
		{
			_mutex->lock_until_succeed(unique_address);
			return { std::adopt_lock, _mutex, unique_address };
		}

		inline bool mutex_t::try_lock(void* unique_address) const
		{
			return _mutex->try_lock(unique_address);
		}

		inline void mutex_t::unlock(void* unique_address) const
		{
			_mutex->unlock(unique_address);
		}
	}
}
