#pragma once

namespace librf
{
	namespace detail
	{
		struct state_event_base_t;

		struct event_v2_impl : public std::enable_shared_from_this<event_v2_impl>
		{
			LIBRF_API event_v2_impl(bool initially) noexcept;
			LIBRF_API ~event_v2_impl();

			bool is_signaled() const noexcept
			{
				return _counter.load(std::memory_order_acquire) > 0;
			}
			LIBRF_API void signal_all() noexcept;
			LIBRF_API void signal() noexcept;
			LIBRF_API void reset() noexcept;

			LIBRF_API void add_wait_list(state_event_base_t* state);
			LIBRF_API void remove_wait_list(state_event_base_t* state);
		public:
			static constexpr bool USE_SPINLOCK = true;

			using lock_type = std::conditional_t<USE_SPINLOCK, spinlock, std::recursive_mutex>;
			using state_event_ptr = counted_ptr<state_event_base_t>;
			using wait_queue_type = intrusive_link_queue<state_event_base_t, state_event_ptr>;

			bool try_wait_one() noexcept
			{
				if (_counter.load(std::memory_order_consume) > 0)
				{
					if (_counter.fetch_add(-1, std::memory_order_acq_rel) > 0)
						return true;
					_counter.fetch_add(1);
				}
				return false;
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

		struct state_event_base_t : public state_base_t
								  , public intrusive_link_node<state_event_base_t, counted_ptr<state_event_base_t>>
		{
			virtual void on_cancel() noexcept = 0;
			virtual bool on_notify(event_v2_impl* eptr) = 0;
			virtual bool on_timeout() = 0;

			template<class _PromiseT> requires(traits::is_promise_v<_PromiseT>)
			scheduler_t* on_await_suspend(coroutine_handle<_PromiseT> handler) noexcept
			{
				_PromiseT& promise = handler.promise();
				auto* parent = promise.get_state();
				scheduler_t* sch = parent->get_scheduler();

				this->_scheduler = sch;
				this->_coro = handler;

				return sch;
			}

			inline void add_timeout_timer(std::chrono::system_clock::time_point tp)
			{
				this->_thandler = this->_scheduler->timer()->add_handler(tp,
					[st = counted_ptr<state_event_base_t>{ this }](bool canceld)
					{
						if (!canceld)
							st->on_timeout();
					});
			}

		protected:
			timer_handler _thandler;
		};


		struct state_event_t : public state_event_base_t
		{
			state_event_t(event_v2_impl*& val)
				: _value(&val)
			{}

			LIBRF_API virtual void on_cancel() noexcept override;
			LIBRF_API virtual bool on_notify(event_v2_impl* eptr) override;
			LIBRF_API virtual bool on_timeout() override;
		protected:
			//_value引用awaitor保存的值，这样可以尽可能减少创建state的可能。而不必进入没有state就没有value实体被用于返回。
			//在调用on_notify()或on_timeout()任意之一后，置为nullptr。
			//这样来保证要么超时了，要么响应了signal的通知了。
			//这个指针在on_notify()和on_timeout()里，当作一个互斥的锁来防止同时进入两个函数
			std::atomic<event_v2_impl**> _value;
		};

		struct state_event_all_t : public state_base_t
		{
			using sub_state_t = std::pair<counted_ptr<state_event_base_t>, detail::event_v2_impl*>;

			state_event_all_t(intptr_t count, bool& val)
				: _counter(count)
				, _result(&val)
			{
				_values.resize(count, sub_state_t{nullptr, nullptr});
			}

			LIBRF_API void on_cancel(intptr_t idx);
			LIBRF_API bool on_notify(event_v2_impl* eptr, intptr_t idx);
			LIBRF_API bool on_timeout();

			template<class _PromiseT> requires(traits::is_promise_v<_PromiseT>)
			scheduler_t* on_await_suspend(coroutine_handle<_PromiseT> handler) noexcept
			{
				_PromiseT& promise = handler.promise();
				auto* parent = promise.get_state();
				scheduler_t* sch = parent->get_scheduler();

				this->_scheduler = sch;
				this->_coro = handler;

				return sch;
			}

			inline void add_timeout_timer(std::chrono::system_clock::time_point tp)
			{
				this->_thandler = this->_scheduler->timer()->add_handler(tp,
					[st = counted_ptr<state_event_all_t>{ this }](bool canceld)
					{
						if (!canceld)
							st->on_timeout();
					});
			}

			std::vector<sub_state_t> _values;
			intptr_t _counter;
			event_v2_impl::lock_type _lock;
		protected:
			timer_handler _thandler;
			bool* _result;
		};

		template<class _StateT>
		struct state_event_proxy_t : public state_event_base_t
		{
			state_event_proxy_t(_StateT* sta, intptr_t idx)
				: _state(sta)
				, _index(idx)
			{
				assert(sta != nullptr);
				assert(idx >= 0);
			}

			void on_cancel() noexcept override { return _state->on_cancel(_index); }
			bool on_notify(event_v2_impl* eptr) override { return _state->on_notify(eptr, _index); }
			bool on_timeout() override { assert(false); return false; }
		private:
			_StateT* _state;
			intptr_t _index;
		};
	}

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
		~awaiter()
		{
			if (_event != nullptr && _state != nullptr)
				_event->remove_wait_list(_state.get());
		}

		bool await_ready() noexcept
		{
			scoped_lock<detail::event_v2_impl::lock_type> lock_(_event->_lock);
			return _event->try_wait_one();
		}

		template<class _PromiseT, class _Timeout> requires(traits::is_promise_v<_PromiseT>)
		bool await_suspend2(coroutine_handle<_PromiseT> handler, const _Timeout& cb)
		{
			(void)cb;
			detail::event_v2_impl* evt = _event;
			scoped_lock<detail::event_v2_impl::lock_type> lock_(evt->_lock);

			if (evt->try_wait_one())
			{
				return false;
			}

			_state = new detail::state_event_t(_event);
			(void)_state->on_await_suspend(handler);

			if constexpr (!std::is_same_v<std::remove_reference_t<_Timeout>, std::nullptr_t>)
			{
				cb();
			}

			evt->add_wait_list(_state.get());

			return true;
		}

		template<class _PromiseT> requires(traits::is_promise_v<_PromiseT>)
		bool await_suspend(coroutine_handle<_PromiseT> handler)
		{
			return await_suspend2(handler, nullptr);
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

	template<class _Btype>
	struct event_t::timeout_awaitor_impl : public _Btype
	{
		template<class... Args>
		timeout_awaitor_impl(clock_type::time_point tp, Args&&... args) noexcept(std::is_nothrow_constructible_v<_Btype, Args&&...>)
			: _Btype(std::forward<Args>(args)...)
			, _tp(tp)
		{}
		template<class _PromiseT> requires(traits::is_promise_v<_PromiseT>)
		bool await_suspend(coroutine_handle<_PromiseT> handler)
		{
			if (!_Btype::await_suspend2(handler, [this]{ this->_state->add_timeout_timer(_tp);}))
			{
				return false;
			}
			return true;
		}
	protected:
		clock_type::time_point _tp;
	};

	struct [[nodiscard]] event_t::timeout_awaiter : timeout_awaitor_impl<awaiter>
	{
		using timeout_awaitor_impl<awaiter>::timeout_awaitor_impl;
	};

	template<class _Rep, class _Period>
	inline event_t::timeout_awaiter event_t::wait_for(const std::chrono::duration<_Rep, _Period>& dt) const noexcept
	{
		clock_type::time_point tp2 = clock_type::now() + std::chrono::duration_cast<clock_type::duration>(dt);
		return { tp2, _event.get() };
	}

	template<class _Clock, class _Duration>
	inline event_t::timeout_awaiter event_t::wait_until(const std::chrono::time_point<_Clock, _Duration>& tp) const noexcept
	{
		clock_type::time_point tp2 = std::chrono::time_point_cast<clock_type::duration>(tp);
		return { tp2, _event.get() };
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
			return _begin == _end;
		}

		template<class _PromiseT, class _Timeout> requires(traits::is_promise_v<_PromiseT>)
		bool await_suspend2(coroutine_handle<_PromiseT> handler, const _Timeout& cb)
		{
			(void)cb;
			using ref_lock_type = std::reference_wrapper<detail::event_v2_impl::lock_type>;
			std::vector<ref_lock_type> lockes;
			lockes.reserve(std::distance(_begin, _end));

			for (auto iter = _begin; iter != _end; ++iter)
			{
				detail::event_v2_impl* evt = (*iter)._event.get();
				lockes.emplace_back(std::ref(evt->_lock));
			}

			batch_lock_t<ref_lock_type> lock_(lockes);

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

			if constexpr (!std::is_same_v<std::remove_reference_t<_Timeout>, std::nullptr_t>)
			{
				cb();
			}

			for (auto iter = _begin; iter != _end; ++iter)
			{
				detail::event_v2_impl* evt = (*iter)._event.get();
				evt->add_wait_list(_state.get());
			}

			return true;
		}

		template<class _PromiseT> requires(traits::is_promise_v<_PromiseT>)
		bool await_suspend(coroutine_handle<_PromiseT> handler)
		{
			return await_suspend2(handler, nullptr);
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
	protected:
		detail::event_v2_impl* _event = nullptr;
		counted_ptr<detail::state_event_t> _state;
		_Iter _begin;
		_Iter _end;
	};

	template<class _Iter>
	requires(_IteratorOfT<_Iter, event_t>)
	auto event_t::wait_any(_Iter begin_, _Iter end_) ->event_t::any_awaiter<_Iter>
	{
		assert(false && "Function is flawed!");
		return { begin_, end_ };
	}

	template<class _Cont>
	requires(_ContainerOfT<_Cont, event_t>)
	auto event_t::wait_any(const _Cont& cnt_) ->event_t::any_awaiter<decltype(std::begin(cnt_))>
	{
		assert(false && "Function is flawed!");
		return { std::begin(cnt_), std::end(cnt_) };
	}



	template<class _Iter>
	struct [[nodiscard]] event_t::timeout_any_awaiter : timeout_awaitor_impl<any_awaiter<_Iter>>
	{
		using timeout_awaitor_impl<any_awaiter<_Iter>>::timeout_awaitor_impl;
	};

	template<class _Rep, class _Period, class _Iter>
	requires(_IteratorOfT<_Iter, event_t>)
	auto event_t::wait_any_for(const std::chrono::duration<_Rep, _Period>& dt, _Iter begin_, _Iter end_)
		->event_t::timeout_any_awaiter<_Iter>
	{
		assert(false && "Function is flawed!");
		clock_type::time_point tp = clock_type::now() + std::chrono::duration_cast<clock_type::duration>(dt);
		return { tp, begin_, end_ };
	}

	template<class _Rep, class _Period, class _Cont>
	requires(_ContainerOfT<_Cont, event_t>)
	auto event_t::wait_any_for(const std::chrono::duration<_Rep, _Period>& dt, const _Cont& cnt_)
		->event_t::timeout_any_awaiter<decltype(std::begin(cnt_))>
	{
		assert(false && "Function is flawed!");
		clock_type::time_point tp = clock_type::now() + std::chrono::duration_cast<clock_type::duration>(dt);
		return { tp, std::begin(cnt_), std::end(cnt_) };
	}



	template<class _Iter>
	struct [[nodiscard]] event_t::all_awaiter
	{
		all_awaiter(_Iter begin, _Iter end) noexcept
			: _begin(begin)
			, _end(end)
		{
		}

		bool await_ready() noexcept
		{
			_value = _begin == _end;
			return _value;
		}

		template<class _PromiseT, class _Timeout> requires(traits::is_promise_v<_PromiseT>)
		bool await_suspend2(coroutine_handle<_PromiseT> handler, const _Timeout& cb)
		{
			(void)cb;
			const intptr_t count = std::distance(_begin, _end);

			using ref_lock_type = std::reference_wrapper<detail::event_v2_impl::lock_type>;
			std::vector<ref_lock_type> lockes;
			lockes.reserve(count + 1);

			for (auto iter = _begin; iter != _end; ++iter)
			{
				detail::event_v2_impl* evt = (*iter)._event.get();
				lockes.push_back(evt->_lock);
			}

			_state = new detail::state_event_all_t(count, _value);
			lockes.push_back(_state->_lock);
			(void)_state->on_await_suspend(handler);

			if constexpr (!std::is_same_v<std::remove_reference_t<_Timeout>, std::nullptr_t>)
			{
				cb();
			}

			batch_lock_t<ref_lock_type> lock_(lockes);

			intptr_t idx = 0;
			for (auto iter = _begin; iter != _end; ++iter, ++idx)
			{
				detail::event_v2_impl* evt = (*iter)._event.get();
				if (evt->try_wait_one())
				{
					--_state->_counter;
					_state->_values[idx].second = evt;
				}
				else
				{
					auto* proxy = new detail::state_event_proxy_t<detail::state_event_all_t>(_state.get(), idx);
					_state->_values[idx] = detail::state_event_all_t::sub_state_t{ proxy, evt };

					evt->add_wait_list(proxy);
				}
			}

			if (_state->_counter == 0)
			{
				_state = nullptr;
				_value = true;

				return false;
			}

			return true;
		}

		template<class _PromiseT> requires(traits::is_promise_v<_PromiseT>)
		bool await_suspend(coroutine_handle<_PromiseT> handler)
		{
			return await_suspend2(handler, nullptr);
		}

		bool await_resume() noexcept
		{
			return _value;
		}
	protected:
		_Iter _begin;
		_Iter _end;
		counted_ptr<detail::state_event_all_t> _state;
		bool _value = false;
	};

	template<class _Iter>
	requires(_IteratorOfT<_Iter, event_t>)
	auto event_t::wait_all(_Iter begin_, _Iter end_) ->all_awaiter<_Iter>
	{
		return { begin_, end_ };
	}

	template<class _Cont>
	requires(_ContainerOfT<_Cont, event_t>)
	auto event_t::wait_all(const _Cont& cnt_) ->all_awaiter<decltype(std::begin(cnt_))>
	{
		return { std::begin(cnt_), std::end(cnt_) };
	}


	template<class _Iter>
	struct [[nodiscard]] event_t::timeout_all_awaiter : timeout_awaitor_impl<all_awaiter<_Iter>>
	{
		using timeout_awaitor_impl<all_awaiter<_Iter>>::timeout_awaitor_impl;
	};

	template<class _Rep, class _Period, class _Iter>
	requires(_IteratorOfT<_Iter, event_t>)
	auto event_t::wait_all_for(const std::chrono::duration<_Rep, _Period>& dt, _Iter begin_, _Iter end_)
		->event_t::timeout_all_awaiter<_Iter>
	{
		clock_type::time_point tp = clock_type::now() + std::chrono::duration_cast<clock_type::duration>(dt);
		return { tp, begin_, end_ };
	}

	template<class _Rep, class _Period, class _Cont>
	requires(_ContainerOfT<_Cont, event_t>)
	auto event_t::wait_all_for(const std::chrono::duration<_Rep, _Period>& dt, const _Cont& cnt_)
		->event_t::timeout_all_awaiter<decltype(std::begin(cnt_))>
	{
		clock_type::time_point tp = clock_type::now() + std::chrono::duration_cast<clock_type::duration>(dt);
		return { tp, std::begin(cnt_), std::end(cnt_) };
	}
}
