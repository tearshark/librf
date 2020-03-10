#pragma once

RESUMEF_NS
{
	namespace detail
	{
		struct state_event_t;

		//仿照cppcoro的event是行不通的。
		//虽然cppcoro的event的触发和等待之间是线程安全的，但是并不能实现只触发指定数量。并且多线程触发之间是不安全的。
		//所以，还得用锁结构来实现(等待实现，今日不空)。
		struct event_v2_impl : public std::enable_shared_from_this<event_v2_impl>
		{
			event_v2_impl(bool initially) noexcept
				: _counter(initially ? 1 : 0)
			{}

			bool is_signaled() const noexcept
			{
				return _counter.load(std::memory_order_acquire) > 0;
			}
			void reset() noexcept
			{
				_counter.store(0, std::memory_order_release);
			}
			void signal_all() noexcept;
			void signal() noexcept;

		public:
			static constexpr bool USE_SPINLOCK = true;

			using lock_type = std::conditional_t<USE_SPINLOCK, spinlock, std::deque<std::recursive_mutex>>;
			using wait_queue_type = intrusive_link_queue<state_event_t>;

			friend struct state_event_t;

			bool try_wait_one() noexcept
			{
				if (_counter.fetch_add(-1, std::memory_order_acq_rel) > 0)
					return true;
				_counter.fetch_add(1);
				return false;
			}

			void add_wait_list(state_event_t* state) noexcept
			{
				assert(state != nullptr);
				_wait_awakes.push_back(state);
			}

			lock_type _lock;									//保证访问本对象是线程安全的
		private:
			std::atomic<intptr_t> _counter;
			wait_queue_type _wait_awakes;						//等待队列

			// No copying/moving
			event_v2_impl(const event_v2_impl&) = delete;
			event_v2_impl(event_v2_impl&&) = delete;
			event_v2_impl& operator=(const event_v2_impl&) = delete;
			event_v2_impl& operator=(event_v2_impl&&) = delete;
		};

		struct state_event_t : public state_base_t
		{
			state_event_t(bool& val)
				: _value(&val)
			{}

			virtual void resume() override;
			virtual bool has_handler() const  noexcept override;

			void on_notify();
			void on_cancel() noexcept;

			//将自己加入到通知链表里
			template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
			void on_await_suspend(coroutine_handle<_PromiseT> handler) noexcept
			{
				_PromiseT& promise = handler.promise();
				auto* parent_state = promise.get_state();
				scheduler_t* sch = parent_state->get_scheduler();

				this->_scheduler = sch;
				this->_coro = handler;
			}
		public:
			//为浸入式单向链表提供的next指针
			state_event_t* _next = nullptr;
			bool* _value;
		};
	}

	namespace event_v2
	{
		struct [[nodiscard]] event_t::awaiter
		{
			awaiter(event_impl_ptr evt) noexcept
				: _event(std::move(evt))
			{
			}

			bool await_ready() noexcept
			{
				return (_value = _event->try_wait_one());
			}
			template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
			bool await_suspend(coroutine_handle<_PromiseT> handler)
			{
				scoped_lock<detail::event_v2_impl::lock_type> lock_(_event->_lock);

				if (_event->try_wait_one())
					return false;

				_state = new detail::state_event_t(_value);
				_state->on_await_suspend(handler);

				_event->add_wait_list(_state.get());

				return true;
			}
			bool await_resume() noexcept
			{
				return _value;
			}
		private:
			std::shared_ptr<detail::event_v2_impl> _event;
			counted_ptr<detail::state_event_t> _state;
			bool _value = false;
		};

		inline void event_t::signal_all() const noexcept
		{
			_event->signal_all();
		}

		inline void event_t::signal() const noexcept
		{
			_event->signal();
		}

		inline void event_t::reset() const noexcept
		{
			_event->reset();
		}

		inline event_t::awaiter event_t::operator co_await() const noexcept
		{
			return { _event };
		}

		inline event_t::awaiter event_t::wait() const noexcept
		{
			return { _event };
		}
	}
}
