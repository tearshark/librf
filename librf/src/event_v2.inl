#pragma once

RESUMEF_NS
{
	namespace detail
	{
		struct state_event_t;

		struct event_v2_impl : public std::enable_shared_from_this<event_v2_impl>
		{
			event_v2_impl(bool initially) noexcept;
			~event_v2_impl();

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
			static constexpr bool USE_LINK_QUEUE = false;

			using lock_type = std::conditional_t<USE_SPINLOCK, spinlock, std::recursive_mutex>;
			using state_event_ptr = counted_ptr<state_event_t>;
			using link_state_queue = intrusive_link_queue<state_event_t, state_event_ptr>;
			using wait_queue_type = std::conditional_t<USE_LINK_QUEUE, link_state_queue, std::list<state_event_ptr>>;

			friend struct state_event_t;

			bool try_wait_one() noexcept
			{
				if (_counter.load(std::memory_order_acquire) > 0)
				{
					if (_counter.fetch_add(-1, std::memory_order_acq_rel) > 0)
						return true;
					_counter.fetch_add(1);
				}
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
			state_event_t(event_v2_impl*& val)
				: _value(&val)
			{}

			virtual void resume() override;
			virtual bool has_handler() const  noexcept override;

			void on_cancel() noexcept;
			bool on_notify(event_v2_impl* eptr);
			bool on_timeout();

			//将自己加入到通知链表里
			template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
			scheduler_t* on_await_suspend(coroutine_handle<_PromiseT> handler) noexcept
			{
				_PromiseT& promise = handler.promise();
				auto* parent = promise.get_state();
				scheduler_t* sch = parent->get_scheduler();

				this->_scheduler = sch;
				this->_coro = handler;

				return sch;
			}

		public:
			typedef spinlock lock_type;

			//为浸入式单向链表提供的next指针
			//counted_ptr<state_event_t> _next = nullptr;
			timer_handler _thandler;
		private:
			//_value引用awaitor保存的值，这样可以尽可能减少创建state的可能。而不必进入没有state就没有value实体被用于返回。
			//在调用on_notify()或on_timeout()任意之一后，置为nullptr。
			//这样来保证要么超时了，要么响应了signal的通知了。
			//这个指针在on_notify()和on_timeout()里，当作一个互斥的锁来防止同时进入两个函数
			std::atomic<event_v2_impl**> _value;
		};
	}

	inline namespace event_v2
	{
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

		struct [[nodiscard]] event_t::awaiter
		{
			awaiter(detail::event_v2_impl* evt) noexcept
				: _event(evt)
			{
			}

			bool await_ready() noexcept
			{
				return _event->try_wait_one();
			}

			template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
			bool await_suspend(coroutine_handle<_PromiseT> handler)
			{
				detail::event_v2_impl* evt = _event;
				scoped_lock<detail::event_v2_impl::lock_type> lock_(evt->_lock);

				if (evt->try_wait_one())
					return false;

				_state = new detail::state_event_t(_event);
				_event = nullptr;
				(void)_state->on_await_suspend(handler);

				evt->add_wait_list(_state.get());

				return true;
			}

			bool await_resume() noexcept
			{
				return _event != nullptr;
			}

		protected:
			detail::event_v2_impl* _event;
			counted_ptr<detail::state_event_t> _state;
		};

		inline event_t::awaiter event_t::operator co_await() const noexcept
		{
			return { _event.get() };
		}

		inline event_t::awaiter event_t::wait() const noexcept
		{
			return { _event.get() };
		}

		struct [[nodiscard]] event_t::timeout_awaiter : public event_t::awaiter
		{
			timeout_awaiter(detail::event_v2_impl* evt, clock_type::time_point tp) noexcept
				: awaiter(evt)
				, _tp(tp)
			{
			}

			template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
			bool await_suspend(coroutine_handle<_PromiseT> handler)
			{
				if (!awaiter::await_suspend(handler))
					return false;

				_PromiseT& promise = handler.promise();
				auto* parent_state = promise.get_state();
				scheduler_t* sch = parent_state->get_scheduler();

				_state->_thandler = sch->timer()->add_handler(_tp, [_state=_state](bool canceld)
					{
						if (!canceld)
							_state->on_timeout();
					});

				return true;
			}
		private:
			clock_type::time_point _tp;
		};

		template<class _Rep, class _Period>
		inline event_t::timeout_awaiter event_t::wait_for(const std::chrono::duration<_Rep, _Period>& dt) const noexcept
		{
			return wait_until_(clock_type::now() + std::chrono::duration_cast<clock_type::duration>(dt));
		}

		template<class _Clock, class _Duration>
		inline event_t::timeout_awaiter event_t::wait_until(const std::chrono::time_point<_Clock, _Duration>& tp) const noexcept
		{
			return wait_until_(std::chrono::time_point_cast<clock_type::duration>(tp));
		}


		template<class _Iter>
		struct [[nodiscard]] event_t::any_awaiter
		{
			any_awaiter(_Iter begin, _Iter end) noexcept
				: _begin(begin)
				, _end(end)
			{
			}

			bool await_ready() noexcept
			{
				if (_begin == _end)
					return true;

				for (auto iter = _begin; iter != _end; ++iter)
				{
					detail::event_v2_impl* evt = (*iter)._event.get();
					if (evt->try_wait_one())
					{
						_event = evt;
						return true;
					}
				}

				return false;
			}

			template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
			bool await_suspend(coroutine_handle<_PromiseT> handler)
			{
				std::vector<detail::event_v2_impl::lock_type> lockes;
				lockes.reserve(std::distance(_begin, _end));

				for (auto iter = _begin; iter != _end; ++iter)
				{
					detail::event_v2_impl* evt = (*iter)._event.get();
					lockes.push_back(evt->_lock);
				}

				scoped_lock_range<detail::event_v2_impl::lock_type> lock_(lockes);

				for (auto iter = _begin; iter != _end; ++iter)
				{
					detail::event_v2_impl* evt = (*iter)._event.get();
					if (evt->try_wait_one())
					{
						_event = evt;
						return false;
					}
				}

				_state = new detail::state_event_t(_event);
				(void)_state->on_await_suspend(handler);

				for (auto iter = _begin; iter != _end; ++iter)
				{
					detail::event_v2_impl* evt = (*iter)._event.get();
					evt->add_wait_list(_state.get());
				}

				return true;
			}

			intptr_t await_resume() noexcept
			{
				if (_begin == _end)
					return 0;
				if (_event == nullptr)
					return -1;

				intptr_t idx = 0;
				for (auto iter = _begin; iter != _end; ++iter, ++idx)
				{
					detail::event_v2_impl* evt = (*iter)._event.get();
					if (evt == _event)
						return idx;
				}

				return -1;
			}
		private:
			detail::event_v2_impl* _event = nullptr;
			counted_ptr<detail::state_event_t> _state;
			_Iter _begin;
			_Iter _end;
		};

		template<class _Iter COMMA_RESUMEF_ENABLE_IF_TYPENAME()>
		RESUMEF_REQUIRES(_IteratorOfT<_Iter, event_t>)
		auto event_t::wait_any(_Iter begin_, _Iter end_) ->event_t::any_awaiter<_Iter>
		{
			return { begin_, end_ };
		}

		template<class _Cont COMMA_RESUMEF_ENABLE_IF_TYPENAME()>
		RESUMEF_REQUIRES(_ContainerOfT<_Cont, event_t>)
		auto event_t::wait_any(_Cont& cnt_) ->event_t::any_awaiter<decltype(std::begin(cnt_))>
		{
			return { std::begin(cnt_), std::end(cnt_) };
		}
	}
}
