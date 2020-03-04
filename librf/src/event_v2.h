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
			event_v2_impl(bool initially = false) noexcept
				: m_state(initially ? this : nullptr)
			{}

			// No copying/moving
			event_v2_impl(const event_v2_impl&) = delete;
			event_v2_impl(event_v2_impl&&) = delete;
			event_v2_impl& operator=(const event_v2_impl&) = delete;
			event_v2_impl& operator=(event_v2_impl&&) = delete;

			bool is_set() const noexcept
			{
				return m_state.load(std::memory_order_acquire) == this;
			}
			void reset() noexcept
			{
				void* oldValue = this;
				m_state.compare_exchange_strong(oldValue, nullptr, std::memory_order_acquire);
			}
			void notify_all() noexcept;		//多线程同时调用notify_one/notify_all是非线程安全的
			void notify_one() noexcept;		//多线程同时调用notify_one/notify_all是非线程安全的

			bool add_notify_list(state_event_t* state) noexcept;
		private:
			mutable std::atomic<void*> m_state;		//event_v2_impl or state_event_t
		};

		struct state_event_t : public state_base_t
		{
			virtual void resume() override;
			virtual bool has_handler() const  noexcept override;

			void on_notify()
			{
				assert(this->_scheduler != nullptr);
				if (this->_coro)
					this->_scheduler->add_generator(this);
			}

			//将自己加入到通知链表里
			template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
			bool event_await_suspend(coroutine_handle<_PromiseT> handler) noexcept
			{
				_PromiseT& promise = handler.promise();
				auto* parent_state = promise.get_state();
				scheduler_t* sch = parent_state->get_scheduler();

				this->_scheduler = sch;
				this->_coro = handler;

				return m_event->add_notify_list(this);
			}

			static state_event_t* _Alloc_state(event_v2_impl * e)
			{
				_Alloc_char _Al;
				size_t _Size = sizeof(state_event_t);
#if RESUMEF_DEBUG_COUNTER
				std::cout << "state_event_t::alloc, size=" << sizeof(state_event_t) << std::endl;
#endif
				char* _Ptr = _Al.allocate(_Size);
				return new(_Ptr) state_event_t(e);
			}
		private:
			friend struct event_v2_impl;

			state_event_t(event_v2_impl * e) noexcept
			{
				if (e != nullptr)
					m_event = e->shared_from_this();
			}
			std::shared_ptr<event_v2_impl> m_event;
			state_event_t* m_next = nullptr;

			virtual void destroy_deallocate() override;
		};
	}

	namespace v2
	{
		struct event_t
		{
			typedef std::shared_ptr<detail::event_v2_impl> event_impl_ptr;
			typedef std::weak_ptr<detail::event_v2_impl> event_impl_wptr;
			typedef std::chrono::system_clock clock_type;

			event_t(bool initially = false);

			void notify_all() const noexcept
			{
				_event->notify_all();
			}
			void notify_one() const noexcept
			{
				_event->notify_one();
			}
			void reset() const noexcept
			{
				_event->reset();
			}

			struct awaiter
			{
				awaiter(detail::event_v2_impl* e) noexcept
					: _event(e)
				{}

				bool await_ready() const noexcept
				{
					return _event->is_set();
				}
				template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
				bool await_suspend(coroutine_handle<_PromiseT> handler) noexcept
				{
					_state = detail::state_event_t::_Alloc_state(_event);
					return _state->event_await_suspend(handler);
				}
				void await_resume() noexcept
				{
				}
			private:
				detail::event_v2_impl * _event;
				counted_ptr<detail::state_event_t> _state;
			};

			awaiter operator co_await() const noexcept
			{
				return { _event.get() };
			}
		private:
			event_impl_ptr _event;
		};
	}

	class async_manual_reset_event
	{
	public:
		async_manual_reset_event(bool initiallySet = false) noexcept
			: m_state(initiallySet ? this : nullptr)
		{}

		// No copying/moving
		async_manual_reset_event(const async_manual_reset_event&) = delete;
		async_manual_reset_event(async_manual_reset_event&&) = delete;
		async_manual_reset_event& operator=(const async_manual_reset_event&) = delete;
		async_manual_reset_event& operator=(async_manual_reset_event&&) = delete;

		bool is_set() const noexcept
		{
			return m_state.load(std::memory_order_acquire) == this;
		}

		struct awaiter;
		awaiter operator co_await() const noexcept;

		void set() noexcept;
		void reset() noexcept
		{
			void* oldValue = this;
			m_state.compare_exchange_strong(oldValue, nullptr, std::memory_order_acquire);
		}
	private:
		friend struct awaiter;

		// - 'this' => set state
		// - otherwise => not set, head of linked list of awaiter*.
		mutable std::atomic<void*> m_state;
	};

	struct async_manual_reset_event::awaiter
	{
		awaiter(const async_manual_reset_event& event) noexcept
			: m_event(event)
		{}

		bool await_ready() const noexcept
		{
			return m_event.is_set();
		}
		bool await_suspend(coroutine_handle<> awaitingCoroutine) noexcept;
		void await_resume() noexcept {}
	private:
		friend class async_manual_reset_event;

		const async_manual_reset_event& m_event;
		coroutine_handle<> m_awaitingCoroutine;
		awaiter* m_next;
	};

	inline async_manual_reset_event::awaiter async_manual_reset_event::operator co_await() const noexcept
	{
		return awaiter{ *this };
	}
}
