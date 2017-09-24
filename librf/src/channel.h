#pragma once

#include "_awaker.h"

namespace resumef
{
	namespace detail
	{
		template<class _Ty>
		struct channel_impl : public std::enable_shared_from_this<channel_impl<_Ty>>
		{
			typedef _awaker<channel_impl<_Ty>, _Ty *, future_error> channel_read_awaker;
			typedef std::shared_ptr<channel_read_awaker> channel_read_awaker_ptr;

			typedef _awaker<channel_impl<_Ty>> channel_write_awaker;
			typedef std::shared_ptr<channel_write_awaker> channel_write_awaker_ptr;
			typedef std::pair<channel_write_awaker_ptr, _Ty> write_tuple_type;
		private:
			spinlock _lock;										//保证访问本对象是线程安全的
			const size_t _max_counter;							//数据队列的容量上限
			std::deque<_Ty> _values;							//数据队列
			std::list<channel_read_awaker_ptr> _read_awakes;	//读队列
			std::list<write_tuple_type> _write_awakes;			//写队列
		public:
			channel_impl(size_t max_counter_)
				:_max_counter(max_counter_)
			{
			}

#if _DEBUG
			const std::deque<_Ty> & debug_queue() const
			{
				return _values;
			}
#endif

			template<class callee_t, class = std::enable_if<!std::is_same<std::remove_cv_t<callee_t>, channel_read_awaker_ptr>::value>>
			auto read(callee_t && awaker)
			{
				return read_(std::make_shared<channel_read_awaker>(std::forward<callee_t>(awaker)));
			}
			template<class callee_t, class = std::enable_if<!std::is_same<std::remove_cv_t<callee_t>, channel_write_awaker_ptr>::value>>
			auto write(callee_t && awaker, const _Ty& val)
			{
				return write_(std::make_shared<channel_write_awaker>(std::forward<callee_t>(awaker)), val);
			}
			template<class callee_t, class = std::enable_if<!std::is_same<std::remove_cv_t<callee_t>, channel_write_awaker_ptr>::value>>
			auto write(callee_t && awaker, _Ty&& val)
			{
				return write_(std::make_shared<channel_write_awaker>(std::forward<callee_t>(awaker)), std::forward<_Ty>(val));
			}

			//如果已经触发了awaker,则返回true
			bool read_(channel_read_awaker_ptr && r_awaker)
			{
				assert(r_awaker);

				scoped_lock<spinlock> lock_(this->_lock);

				bool ret_value;
				if (_values.size() > 0)
				{
					//如果数据队列有数据，则可以直接读数据
					auto val = std::move(_values.front());
					_values.pop_front();

					r_awaker->awake(this, 1, &val, future_error::none);
					ret_value = true;
				}
				else
				{
					//否则，将“读等待”放入“读队列”
					_read_awakes.push_back(r_awaker);
					ret_value = false;
				}

				//如果已有写队列，则唤醒一个“写等待”
				awake_one_writer_();

				return ret_value;
			}

			void write_(channel_write_awaker_ptr && w_awaker, const _Ty& val)
			{
				assert(w_awaker);
				scoped_lock<spinlock> lock_(this->_lock);

				//如果满了，则不添加到数据队列，而是将“写等待”和值，放入“写队列”
				bool is_full = _values.size() >= _max_counter;
				if (is_full)
					_write_awakes.push_back(std::make_pair(std::forward<channel_write_awaker_ptr>(w_awaker), val));
				else
					_values.push_back(val);

				//如果已有读队列，则唤醒一个“读等待”
				awake_one_reader_();

				//触发 没有放入“写队列”的“写等待”
				if (!is_full) w_awaker->awake(this, 1);
			}

			void write_(channel_write_awaker_ptr && w_awaker, _Ty&& val)
			{
				assert(w_awaker);
				scoped_lock<spinlock> lock_(this->_lock);

				//如果满了，则不添加到数据队列，而是将“写等待”和值，放入“写队列”
				bool is_full = _values.size() >= _max_counter;
				if (is_full)
					_write_awakes.push_back(std::make_pair(std::forward<channel_write_awaker_ptr>(w_awaker), std::forward<_Ty>(val)));
				else
					_values.push_back(std::forward<_Ty>(val));

				//如果已有读队列，则唤醒一个“读等待”
				awake_one_reader_();

				//触发 没有放入“写队列”的“写等待”
				if(!is_full) w_awaker->awake(this, 1);
			}

		private:
			void awake_one_reader_()
			{
				//assert(!(_read_awakes.size() >= 0 && _values.size() == 0));

				for (auto iter = _read_awakes.begin(); iter != _read_awakes.end(); )
				{
					auto r_awaker = *iter;
					iter = _read_awakes.erase(iter);

					if (r_awaker->awake(this, 1, _values.size() ? &_values.front() : nullptr, future_error::read_before_write))
					{
						if(_values.size()) _values.pop_front();

						//唤醒一个“读等待”后，尝试唤醒一个“写等待”，以处理“数据队列”满后的“写等待”
						awake_one_writer_();
						break;
					}
				}
			}

			void awake_one_writer_()
			{
				for (auto iter = _write_awakes.begin(); iter != _write_awakes.end(); )
				{
					auto w_awaker = std::move(*iter);
					iter = _write_awakes.erase(iter);

					if (w_awaker.first->awake(this, 1))
					{
						//一个“写等待”唤醒后，将“写等待”绑定的值，放入“数据队列”
						_values.push_back(std::move(w_awaker.second));
						break;
					}
				}
			}

			channel_impl(const channel_impl &) = delete;
			channel_impl(channel_impl &&) = delete;
			channel_impl & operator = (const channel_impl &) = delete;
			channel_impl & operator = (channel_impl &&) = delete;
		};
	}

	template<class _Ty>
	struct channel_t
	{
		typedef detail::channel_impl<_Ty> channel_impl_type;
		typedef typename channel_impl_type::channel_read_awaker channel_read_awaker;
		typedef typename channel_impl_type::channel_write_awaker channel_write_awaker;

		typedef std::shared_ptr<channel_impl_type> channel_impl_ptr;
		typedef std::weak_ptr<channel_impl_type> channel_impl_wptr;
		typedef std::chrono::system_clock clock_type;
	private:
		channel_impl_ptr _chan;
	public:
		channel_t(size_t max_counter = 0)
			:_chan(std::make_shared<channel_impl_type>(max_counter))
		{
			
		}

		awaitable_t<bool> write(_Ty&& val) const
		{
			awaitable_t<bool> awaitable;

			auto awaker = std::make_shared<channel_write_awaker>(
				[st = awaitable._state](channel_impl_type * chan) -> bool
			{
				st->set_value(chan ? true : false);
				return true;
			});
			_chan->write_(std::move(awaker), std::forward<_Ty>(val));

			return awaitable;
		}
		awaitable_t<bool> write(const _Ty& val) const
		{
			awaitable_t<bool> awaitable;

			auto awaker = std::make_shared<channel_write_awaker>(
				[st = awaitable._state](channel_impl_type * chan) -> bool
			{
				st->set_value(chan ? true : false);
				return true;
			});
			_chan->write_(std::move(awaker), val);

			return awaitable;
		}
		awaitable_t<_Ty> read() const
		{
			awaitable_t<_Ty> awaitable;

			auto awaker = std::make_shared<channel_read_awaker>(
				[st = awaitable._state](channel_impl_type *, _Ty * val, future_error fe) -> bool
			{
				if(val)
					st->set_value(std::move(*val));
				else
					st->throw_exception(channel_exception{ fe });

				return true;
			});
			_chan->read_(std::move(awaker));

			return awaitable;
		}

		awaitable_t<bool> operator << (_Ty&& val) const
		{
			return std::move(write(std::forward<_Ty>(val)));
		}
		awaitable_t<bool> operator << (const _Ty& val) const
		{
			return std::move(write(val));
		}

		awaitable_t<_Ty> operator co_await () const
		{
			return read();
		}

#if _DEBUG
		const auto & debug_queue() const
		{
			return _chan->debug_queue();
		}
#endif

		channel_t(const channel_t &) = default;
		channel_t(channel_t &&) = default;
		channel_t & operator = (const channel_t &) = default;
		channel_t & operator = (channel_t &&) = default;
	};


	typedef channel_t<bool>	semaphore_t;
}
