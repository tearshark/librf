#pragma once
#include "intrusive_link_queue.h"

RESUMEF_NS
{
namespace detail
{
	template<class _Ty, class _Opty>
	struct channel_impl_v2 : public std::enable_shared_from_this<channel_impl_v2<_Ty, _Opty>>
	{
		using value_type = _Ty;
		using optional_type = _Opty;

		struct state_channel_t : public state_base_t
		{
			state_channel_t(channel_impl_v2* ch) noexcept
				: _channel(ch->shared_from_this())
			{
			}

			virtual void resume() override
			{
				coroutine_handle<> handler = _coro;
				if (handler)
				{
					_coro = nullptr;
					_scheduler->del_final(this);
					handler.resume();
				}
			}

			virtual bool has_handler() const  noexcept override
			{
				return (bool)_coro;
			}

			void on_notify()
			{
				assert(this->_scheduler != nullptr);
				if (this->_coro)
					this->_scheduler->add_generator(this);
			}

			void on_cancel() noexcept
			{
				this->_coro = nullptr;
			}

			template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
			void on_await_suspend(coroutine_handle<_PromiseT> handler) noexcept
			{
				_PromiseT& promise = handler.promise();
				auto* parent_state = promise.get_state();
				scheduler_t* sch = parent_state->get_scheduler();

				this->_scheduler = sch;
				this->_coro = handler;
			}

			void on_await_resume()
			{
				if (_error != error_code::none)
				{
					std::rethrow_exception(std::make_exception_ptr(channel_exception{ _error }));
				}
			}

			friend channel_impl_v2;
		public:
			state_channel_t* _next = nullptr;
		protected:
			std::shared_ptr<channel_impl_v2> _channel;
			error_code _error = error_code::none;
		};

		struct state_read_t : public state_channel_t
		{
			state_read_t(channel_impl_v2* ch, optional_type& val) noexcept
				: state_channel_t(ch)
				, _value(&val)
			{
			}
		protected:
			friend channel_impl_v2;
			optional_type* _value;
		};

		struct state_write_t : public state_channel_t
		{
			state_write_t(channel_impl_v2* ch, value_type& val) noexcept
				: state_channel_t(ch)
				, _value(&val)
			{
			}
		protected:
			friend channel_impl_v2;
			value_type* _value;
		};

		channel_impl_v2(size_t max_counter_);

		bool try_read(optional_type& val);
		bool try_read_nolock(optional_type& val);
		void add_read_list_nolock(state_read_t* state);

		bool try_write(value_type& val);
		bool try_write_nolock(value_type& val);
		void add_write_list_nolock(state_write_t* state);

		size_t capacity() const noexcept
		{
			return _max_counter;
		}
	private:
		auto try_pop_reader_()->state_read_t*;
		auto try_pop_writer_()->state_write_t*;
		void awake_one_reader_();
		bool awake_one_reader_(value_type& val);

		void awake_one_writer_();
		bool awake_one_writer_(optional_type& val);

		channel_impl_v2(const channel_impl_v2&) = delete;
		channel_impl_v2(channel_impl_v2&&) = delete;
		channel_impl_v2& operator = (const channel_impl_v2&) = delete;
		channel_impl_v2& operator = (channel_impl_v2&&) = delete;

		static constexpr bool USE_SPINLOCK = true;
		static constexpr bool USE_RING_QUEUE = true;
		static constexpr bool USE_LINK_QUEUE = true;

		//using queue_type = std::conditional_t<USE_RING_QUEUE, ring_queue_spinlock<value_type, false, uint32_t>, std::deque<value_type>>;
		using queue_type = std::conditional_t<USE_RING_QUEUE, ring_queue<value_type, false, uint32_t>, std::deque<value_type>>;
		using read_queue_type = std::conditional_t<USE_LINK_QUEUE, intrusive_link_queue<state_channel_t>, std::list<state_read_t*>>;
		using write_queue_type = std::conditional_t<USE_LINK_QUEUE, intrusive_link_queue<state_channel_t>, std::list<state_write_t*>>;

		const size_t _max_counter;							//数据队列的容量上限

	public:
		using lock_type = std::conditional_t<USE_SPINLOCK, spinlock, std::deque<std::recursive_mutex>>;
		lock_type _lock;									//保证访问本对象是线程安全的
	private:
		queue_type _values;									//数据队列
		read_queue_type _read_awakes;						//读队列
		write_queue_type _write_awakes;						//写队列
	};

	template<class _Ty, class _Opty>
	channel_impl_v2<_Ty, _Opty>::channel_impl_v2(size_t max_counter_)
		: _max_counter(max_counter_)
		, _values(USE_RING_QUEUE ? max_counter_ : 0)
	{
	}

	template<class _Ty, class _Opty>
	inline bool channel_impl_v2<_Ty, _Opty>::try_read(optional_type& val)
	{
		scoped_lock<lock_type> lock_(this->_lock);
		return try_read_nolock(val);
	}

	template<class _Ty, class _Opty>
	bool channel_impl_v2<_Ty, _Opty>::try_read_nolock(optional_type& val)
	{
		if constexpr (USE_RING_QUEUE)
		{
			if (_values.try_pop(val))
			{
				awake_one_writer_();
				return true;
			}
		}
		else
		{
			if (_values.size() > 0)
			{
				val = std::move(_values.front());
				_values.pop_front();
				awake_one_writer_();

				return true;
			}
		}

		return awake_one_writer_(val);
	}

	template<class _Ty, class _Opty>
	inline void channel_impl_v2<_Ty, _Opty>::add_read_list_nolock(state_read_t* state)
	{
		assert(state != nullptr);
		_read_awakes.push_back(state);
	}

	template<class _Ty, class _Opty>
	inline bool channel_impl_v2<_Ty, _Opty>::try_write(value_type& val)
	{
		scoped_lock<lock_type> lock_(this->_lock);
		return try_write_nolock(val);
	}

	template<class _Ty, class _Opty>
	bool channel_impl_v2<_Ty, _Opty>::try_write_nolock(value_type& val)
	{
		if constexpr (USE_RING_QUEUE)
		{
			if (_values.try_push(std::move(val)))
			{
				awake_one_reader_();
				return true;
			}
		}
		else
		{
			if (_values.size() < _max_counter)
			{
				_values.push_back(std::move(val));
				awake_one_reader_();

				return true;
			}
		}

		return awake_one_reader_(val);
	}

	template<class _Ty, class _Opty>
	inline void channel_impl_v2<_Ty, _Opty>::add_write_list_nolock(state_write_t* state)
	{
		assert(state != nullptr);
		_write_awakes.push_back(state);
	}

	template<class _Ty, class _Opty>
	auto channel_impl_v2<_Ty, _Opty>::try_pop_reader_()->state_read_t*
	{
		if constexpr (USE_LINK_QUEUE)
		{
			return reinterpret_cast<state_read_t*>(_read_awakes.try_pop());
		}
		else
		{
			if (!_read_awakes.empty())
			{
				state_write_t* state = _read_awakes.front();
				_read_awakes.pop_front();
				return state;
			}
			return nullptr;
		}
	}

	template<class _Ty, class _Opty>
	auto channel_impl_v2<_Ty, _Opty>::try_pop_writer_()->state_write_t*
	{
		if constexpr (USE_LINK_QUEUE)
		{
			return reinterpret_cast<state_write_t*>(_write_awakes.try_pop());
		}
		else
		{
			if (!_write_awakes.empty())
			{
				state_write_t* state = _write_awakes.front();
				_write_awakes.pop_front();
				return state;
			}
			return nullptr;
		}
	}

	template<class _Ty, class _Opty>
	void channel_impl_v2<_Ty, _Opty>::awake_one_reader_()
	{
		state_read_t* state = try_pop_reader_();
		if (state != nullptr)
		{
			if constexpr (USE_RING_QUEUE)
			{
				if (!_values.try_pop(*state->_value))
					state->_error = error_code::read_before_write;
			}
			else
			{
				if (_values.size() > 0)
				{
					*state->_value = std::move(_values.front());
					_values.pop_front();
				}
				else
				{
					state->_error = error_code::read_before_write;
				}
			}

			state->on_notify();
		}
	}

	template<class _Ty, class _Opty>
	bool channel_impl_v2<_Ty, _Opty>::awake_one_reader_(value_type& val)
	{
		state_read_t* state = try_pop_reader_();
		if (state != nullptr)
		{
			*state->_value = std::move(val);

			state->on_notify();
			return true;
		}
		return false;
	}

	template<class _Ty, class _Opty>
	void channel_impl_v2<_Ty, _Opty>::awake_one_writer_()
	{
		state_write_t* state = try_pop_writer_();
		if (state != nullptr)
		{
			if constexpr (USE_RING_QUEUE)
			{
				bool ret = _values.try_push(std::move(*state->_value));
				(void)ret;
				assert(ret);
			}
			else
			{
				assert(_values.size() < _max_counter);
				_values.push_back(*state->_value);
			}

			state->on_notify();
		}
	}

	template<class _Ty, class _Opty>
	bool channel_impl_v2<_Ty, _Opty>::awake_one_writer_(optional_type& val)
	{
		state_write_t* writer = try_pop_writer_();
		if (writer != nullptr)
		{
			val = std::move(*writer->_value);

			writer->on_notify();
			return true;
		}
		return false;
	}

}	//namespace detail

inline namespace channel_v2
{
	//如果channel缓存的元素不能凭空产生，或者产生代价较大，则推荐第二个模板参数使用true。从而减小不必要的开销。
	template<class _Ty = bool, bool _Option = false>
	struct channel_t
	{
		using value_type = _Ty;

		static constexpr bool use_option = _Option;
		using optional_type = std::conditional_t<use_option, std::optional<value_type>, value_type>;
		using channel_type = detail::channel_impl_v2<value_type, optional_type>;
		using lock_type = typename channel_type::lock_type;

		channel_t(size_t max_counter = 0)
			:_chan(std::make_shared<channel_type>(max_counter))
		{

		}

		size_t capacity() const noexcept
		{
			return _chan->capacity();
		}

		struct [[nodiscard]] read_awaiter
		{
			using state_type = typename channel_type::state_read_t;

			read_awaiter(channel_type* ch) noexcept
				: _channel(ch)
			{}

			~read_awaiter()
			{//为了不在协程中也能正常使用
				if (_channel != nullptr)
					_channel->try_read(_value);
			}

			bool await_ready() const noexcept
			{
				return false;
			}
			template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
			bool await_suspend(coroutine_handle<_PromiseT> handler)
			{
				scoped_lock<lock_type> lock_(_channel->_lock);

				if (_channel->try_read_nolock(_value))
				{
					_channel = nullptr;
					return false;
				}

				_state = new state_type(_channel, _value);
				_state->on_await_suspend(handler);
				_channel->add_read_list_nolock(_state.get());
				_channel = nullptr;

				return true;
			}
			value_type await_resume()
			{
				if (_state.get() != nullptr)
					_state->on_await_resume();

				if constexpr(use_option)
					return std::move(_value).value();
				else
					return std::move(_value);
			}
		private:
			channel_type* _channel;
			counted_ptr<state_type> _state;
			mutable optional_type _value;
		};

		read_awaiter operator co_await() const noexcept
		{
			return { _chan.get() };
		}
		read_awaiter read() const noexcept
		{
			return { _chan.get() };
		}

		struct [[nodiscard]] write_awaiter
		{
			using state_type = typename channel_type::state_write_t;

			write_awaiter(channel_type* ch, value_type val) noexcept(std::is_move_constructible_v<value_type>)
				: _channel(ch)
				, _value(std::move(val))
			{}

			~write_awaiter()
			{//为了不在协程中也能正常使用
				if (_channel != nullptr)
					_channel->try_write(static_cast<value_type&>(_value));
			}

			bool await_ready() const noexcept
			{
				return false;
			}
			template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
			bool await_suspend(coroutine_handle<_PromiseT> handler)
			{
				scoped_lock<lock_type> lock_(_channel->_lock);

				if (_channel->try_write_nolock(static_cast<value_type&>(_value)))
				{
					_channel = nullptr;
					return false;
				}

				_state = new state_type(_channel, static_cast<value_type&>(_value));
				_state->on_await_suspend(handler);
				_channel->add_write_list_nolock(_state.get());
				_channel = nullptr;

				return true;
			}
			void await_resume()
			{
				if (_state.get() != nullptr)
					_state->on_await_resume();
			}
		private:
			channel_type* _channel;
			counted_ptr<state_type> _state;
			mutable value_type _value;
		};

		template<class U>
		write_awaiter write(U&& val) const noexcept(std::is_move_constructible_v<U>)
		{
			return write_awaiter{ _chan.get(), std::move(val) };
		}

		template<class U>
		write_awaiter operator << (U&& val) const noexcept(std::is_move_constructible_v<U>)
		{
			return write_awaiter{ _chan.get(), std::move(val) };
		}

		channel_t(const channel_t&) = default;
		channel_t(channel_t&&) = default;
		channel_t& operator = (const channel_t&) = default;
		channel_t& operator = (channel_t&&) = default;
	private:
		std::shared_ptr<channel_type> _chan;
	};

	//不支持channel_t<void>
	template<bool _Option>
	struct channel_t<void, _Option>
	{
	};

	using semaphore_t = channel_t<bool>;

}	//namespace v2
}	//RESUMEF_NS
