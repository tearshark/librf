#pragma once

RESUMEF_NS
{
	namespace detail
	{
		struct state_mutex_base_t : public state_base_t
		{
			virtual void resume() override;
			virtual bool has_handler() const  noexcept override;
			virtual state_base_t* get_parent() const noexcept override;

			virtual void on_cancel() noexcept = 0;
			virtual bool on_notify(mutex_v2_impl* eptr) = 0;
			virtual bool on_timeout() = 0;

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

			inline void add_timeout_timer(std::chrono::system_clock::time_point tp)
			{
				this->_thandler = this->_scheduler->timer()->add_handler(tp,
					[st = counted_ptr<state_mutex_base_t>{ this }](bool canceld)
					{
						if (!canceld)
							st->on_timeout();
					});
			}

			timer_handler _thandler;
		protected:
			state_base_t* _root;
		};

		struct state_mutex_t : public state_mutex_base_t
		{
			state_mutex_t(mutex_v2_impl*& val)
				: _value(&val)
			{}

			virtual void on_cancel() noexcept override;
			virtual bool on_notify(mutex_v2_impl* eptr) override;
			virtual bool on_timeout() override;
		public:
			timer_handler _thandler;
		protected:
			std::atomic<mutex_v2_impl**> _value;
		};

		struct state_mutex_all_t : public state_event_base_t
		{
			state_mutex_all_t(intptr_t count, bool& val)
				: _counter(count)
				, _value(&val)
			{}

			virtual void on_cancel() noexcept override;
			virtual bool on_notify(event_v2_impl* eptr) override;
			virtual bool on_timeout() override;
		public:
			timer_handler _thandler;
			std::atomic<intptr_t> _counter;
		protected:
			bool* _value;
		};

		struct mutex_v2_impl : public std::enable_shared_from_this<mutex_v2_impl>
		{
			using clock_type = std::chrono::system_clock;

			mutex_v2_impl() {}

			inline void* owner() const noexcept
			{
				return _owner.load(std::memory_order_acquire);
			}

			bool try_lock(void* sch);					//内部加锁
			bool try_lock_until(clock_type::time_point tp, void* sch);	//内部加锁
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

		struct _MutexAddressAssembleT
		{
		private:
			void* _Address;
		public:
			std::vector<mutex_t> _Lks;

			template<class... _Mtxs>
			_MutexAddressAssembleT(void* unique_address, _Mtxs&... mtxs)
				: _Address(unique_address)
				, _Lks({ mtxs... })
			{}
			size_t size() const
			{
				return _Lks.size();
			}
			mutex_t& operator[](int _Idx)
			{
				return _Lks[_Idx];
			}
			void _Lock_ref(mutex_t& _LkN) const
			{
				return _LkN.lock(_Address);
			}
			bool _Try_lock_ref(mutex_t& _LkN) const
			{
				return _LkN.try_lock(_Address);
			}
			void _Unlock_ref(mutex_t& _LkN) const
			{
				_LkN.unlock(_Address);
			}
			void _Yield() const
			{
				std::this_thread::yield();
			}
			void _ReturnValue() const noexcept {}
			template<class U>
			U _ReturnValue(U v) const noexcept
			{
				return v;
			}
		};

#define LOCK_ASSEMBLE_NAME(fnName) mutex_lock_await_##fnName
#define LOCK_ASSEMBLE_AWAIT(a) co_await (a)
#define LOCK_ASSEMBLE_RETURN(a) co_return (a)
#include "without_deadlock_assemble.inl"
#undef LOCK_ASSEMBLE_NAME
#undef LOCK_ASSEMBLE_AWAIT
#undef LOCK_ASSEMBLE_RETURN
	}

	inline namespace mutex_v2
	{
		template<>
		struct [[nodiscard]] scoped_unlock_t<mutex_t>
		{
			typedef std::shared_ptr<detail::mutex_v2_impl> mutex_impl_ptr;

			scoped_unlock_t() {}

			//此函数，应该在try_lock()获得锁后使用
			//或者在协程里，由awaiter使用
			scoped_unlock_t(std::adopt_lock_t, void* sch, mutex_impl_ptr mtx)
				: _mutex(std::move(mtx))
				, _owner(sch)
			{}

			//此函数，适合在非协程里使用
			scoped_unlock_t(void* sch, mutex_impl_ptr mtx)
				: _mutex(std::move(mtx))
				, _owner(sch)
			{
				if (_mutex != nullptr)
					_mutex->lock_until_succeed(sch);
			}


			scoped_unlock_t(std::adopt_lock_t, void* sch, const mutex_t& mtx)
				: scoped_unlock_t(std::adopt_lock, sch, mtx._mutex)
			{}
			scoped_unlock_t(void* sch, const mutex_t& mtx)
				: scoped_unlock_t(sch, mtx._mutex)
			{}

			~scoped_unlock_t()
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

			scoped_unlock_t(const scoped_unlock_t&) = delete;
			scoped_unlock_t& operator = (const scoped_unlock_t&) = delete;
			scoped_unlock_t(scoped_unlock_t&& _Right) = default;
			scoped_unlock_t& operator = (scoped_unlock_t&& _Right) = default;
		private:
			mutex_impl_ptr _mutex;
			void* _owner;
		};

		struct mutex_t::lock_awaiter
		{
			lock_awaiter(detail::mutex_v2_impl* mtx) noexcept
				: _mutex(mtx)
			{
				assert(_mutex != nullptr);
			}

			~lock_awaiter() noexcept(false)
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
				assert(_root != nullptr);
				assert(_root->get_parent() == nullptr);

				scoped_lock<detail::mutex_v2_impl::lock_type> lock_(_mutex->_lock);
				if (_mutex->try_lock_lockless(_root))
					return false;

				_state = new detail::state_mutex_t(_mutex);
				_state->on_await_suspend(handler, parent->get_scheduler(), _root);

				_mutex->add_wait_list_lockless(_state.get());

				return true;
			}
		protected:
			detail::mutex_v2_impl* _mutex;
			counted_ptr<detail::state_mutex_t> _state;
			state_base_t* _root = nullptr;
		};

		struct [[nodiscard]] mutex_t::awaiter : public lock_awaiter
		{
			using lock_awaiter::lock_awaiter;

			scoped_unlock_t<mutex_t> await_resume() noexcept
			{
				mutex_impl_ptr mtx = _mutex ? _mutex->shared_from_this() : nullptr;
				_mutex = nullptr;

				return { std::adopt_lock, _root, mtx };
			}
		};

		inline mutex_t::awaiter mutex_t::operator co_await() const noexcept
		{
			return { _mutex.get() };
		}

		inline mutex_t::awaiter mutex_t::lock() const noexcept
		{
			return { _mutex.get() };
		}

		struct mutex_t::manual_awaiter : public lock_awaiter
		{
			using lock_awaiter::lock_awaiter;
			void await_resume() noexcept
			{
				_mutex = nullptr;
			}
		};

		inline mutex_t::manual_awaiter mutex_t::lock(adopt_manual_unlock_t) const noexcept
		{
			return { _mutex.get() };
		}

		inline bool mutex_t::is_locked() const
		{
			return _mutex->owner() != nullptr;
		}

		struct [[nodiscard]] mutex_t::try_awaiter
		{
			try_awaiter(detail::mutex_v2_impl* mtx) noexcept
				: _mutex(mtx)
			{
				assert(_mutex != nullptr);
			}
			~try_awaiter() noexcept(false)
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
				if (!_mutex->try_lock(parent->get_root()))
					_mutex = nullptr;

				return false;
			}

			bool await_resume() noexcept
			{
				detail::mutex_v2_impl* mtx = _mutex;
				_mutex = nullptr;
				return mtx != nullptr;
			}
		protected:
			detail::mutex_v2_impl* _mutex;
		};

		inline mutex_t::try_awaiter mutex_t::try_lock() const noexcept
		{
			return { _mutex.get() };
		}

		struct [[nodiscard]] mutex_t::unlock_awaiter
		{
			unlock_awaiter(detail::mutex_v2_impl* mtx) noexcept
				: _mutex(mtx)
			{
				assert(_mutex != nullptr);
			}
			~unlock_awaiter() noexcept(false)
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
				_mutex->unlock(parent->get_root());

				return false;
			}

			void await_resume() noexcept
			{
				_mutex = nullptr;
			}
		protected:
			detail::mutex_v2_impl* _mutex;
		};

		inline mutex_t::unlock_awaiter mutex_t::unlock() const noexcept
		{
			return { _mutex.get() };
		}




		struct [[nodiscard]] mutex_t::timeout_awaiter : public event_t::timeout_awaitor_impl<awaiter>
		{
			timeout_awaiter(clock_type::time_point tp, detail::mutex_v2_impl * mtx) noexcept
				: event_t::timeout_awaitor_impl<mutex_t::awaiter>(tp, mtx)
			{}

			bool await_resume() noexcept
			{
				detail::mutex_v2_impl* mtx = this->_mutex;
				this->_mutex = nullptr;
				return mtx != nullptr;
			}
		};

		template <class _Rep, class _Period>
		inline mutex_t::timeout_awaiter mutex_t::try_lock_until(const std::chrono::time_point<_Rep, _Period>& tp) const noexcept
		{
			return { tp, _mutex.get() };
		}

		template <class _Rep, class _Period>
		inline mutex_t::timeout_awaiter mutex_t::try_lock_for(const std::chrono::duration<_Rep, _Period>& dt) const noexcept
		{
			auto tp = clock_type::now() + std::chrono::duration_cast<clock_type::duration>(dt);
			return { tp, _mutex.get() };
		}


		inline void mutex_t::lock(void* unique_address) const
		{
			assert(unique_address != nullptr);
			_mutex->lock_until_succeed(unique_address);
		}

		inline bool mutex_t::try_lock(void* unique_address) const
		{
			assert(unique_address != nullptr);
			return _mutex->try_lock(unique_address);
		}

		template <class _Rep, class _Period>
		inline bool mutex_t::try_lock_for(const std::chrono::duration<_Rep, _Period>& dt, void* unique_address)
		{
			assert(unique_address != nullptr);
			return _mutex->try_lock_until(clock_type::now() + std::chrono::duration_cast<clock_type::duration>(dt), unique_address);
		}

		template <class _Rep, class _Period>
		inline bool mutex_t::try_lock_until(const std::chrono::time_point<_Rep, _Period>& tp, void* unique_address)
		{
			assert(unique_address != nullptr);
			return _mutex->try_lock_until(std::chrono::time_point_cast<clock_type::time_point>(tp), unique_address);
		}

		inline void mutex_t::unlock(void* unique_address) const
		{
			assert(unique_address != nullptr);
			_mutex->unlock(unique_address);
		}



		struct mutex_t::_MutexAwaitAssembleT
		{
		public:
			std::vector<mutex_t> _mutex;
			void* _owner;

			template<class... _Mtxs>
			_MutexAwaitAssembleT(void* unique_address, _Mtxs&... mtxs)
				: _mutex({ mtxs... })
				, _owner(unique_address)
			{}
			size_t size() const
			{
				return _mutex.size();
			}
			mutex_t& operator[](int _Idx)
			{
				return _mutex[_Idx];
			}
			auto _Lock_ref(mutex_t& _LkN) const
			{
				return _LkN.lock(adopt_manual_unlock);
			}
			auto _Try_lock_ref(mutex_t& _LkN) const
			{
				return _LkN.try_lock();
			}
			void _Unlock_ref(mutex_t& _LkN) const
			{
				_LkN.unlock(_owner);
			}
			future_t<> _Yield() const
			{
				for (int cnt = rand() % (1 + _mutex.size()); cnt >= 0; --cnt)
				{
					std::this_thread::yield();	//还要考虑多线程里运行的情况
					co_await ::resumef::yield();
				}
			}
			future_t<> _ReturnValue() const;
			template<class U>
			future_t<U> _ReturnValue(U v) const;

			_MutexAwaitAssembleT(const _MutexAwaitAssembleT&) = default;
			_MutexAwaitAssembleT& operator = (const _MutexAwaitAssembleT&) = default;
			_MutexAwaitAssembleT(_MutexAwaitAssembleT&& _Right) = default;
			_MutexAwaitAssembleT& operator = (_MutexAwaitAssembleT&& _Right) = default;
		};

		template<class... _Mtxs>
		struct [[nodiscard]] scoped_unlock_t
		{
			mutex_t::_MutexAwaitAssembleT _MAA;

			//此函数，应该在try_lock()获得锁后使用
			//或者在协程里，由awaiter使用
			template<class... U>
			scoped_unlock_t(void* sch, U&&... mtxs)
				: _MAA(sch, std::forward<U>(mtxs)...)
			{
				detail::scoped_lock_range_lock_impl::_Lock_range(_MAA);
			}
			template<class... U>
			scoped_unlock_t(std::adopt_lock_t, void* sch, U&&... mtxs)
				: _MAA(sch, std::forward<U>(mtxs)...)
			{}

			~scoped_unlock_t()
			{
				if (_MAA._owner != nullptr)
				{
					for (mutex_t& mtx : _MAA._mutex)
						mtx.unlock(_MAA._owner);
				}
			}

			inline void unlock() noexcept
			{
				if (_MAA._owner != nullptr)
				{
					for (mutex_t& mtx : _MAA._mutex)
						mtx.unlock(_MAA._owner);
					_MAA._owner = nullptr;
				}
			}

			scoped_unlock_t(const scoped_unlock_t&) = delete;
			scoped_unlock_t& operator = (const scoped_unlock_t&) = delete;
			scoped_unlock_t(scoped_unlock_t&& _Right) = default;
			scoped_unlock_t& operator = (scoped_unlock_t&& _Right) = default;
		};

		template<class... _Mtxs>
		scoped_unlock_t()->scoped_unlock_t<scoped_unlock_t<_Mtxs...>&&>;

		template<class... _Mtxs, typename>
		inline future_t<scoped_unlock_t<_Mtxs...>> mutex_t::lock(_Mtxs&... mtxs)
		{
			scoped_unlock_t<_Mtxs...> unlock_guard{ std::adopt_lock, root_state(), mtxs... };
			co_await detail::mutex_lock_await_lock_impl::_Lock_range(unlock_guard._MAA);
			co_return std::move(unlock_guard);
		}

		template<class... _Mtxs, typename>
		inline scoped_unlock_t<_Mtxs...> mutex_t::lock(void* unique_address, _Mtxs&... mtxs)
		{
			assert(unique_address != nullptr);
			return { unique_address, mtxs... };
		}

		template<class... _Mtxs, typename>
		inline void mutex_t::unlock(void* unique_address, _Mtxs&... mtxs)
		{
			assert(unique_address != nullptr);

			int _Ignored[] = { (mtxs.unlock(unique_address), 0)... };
			(void)_Ignored;
		}
	}
}

