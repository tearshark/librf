#include "event.h"
#include <assert.h>
#include <mutex>
#include "scheduler.h"

namespace resumef
{
	namespace detail
	{
		event_impl::event_impl(intptr_t initial_counter_)
			: _counter(initial_counter_)
		{
		}

		void event_impl::signal()
		{
			scoped_lock<spinlock> lock_(this->_lock);

			++this->_counter;

			for (auto iter = this->_awakes.begin(); iter != this->_awakes.end(); )
			{
				auto awaker = *iter;
				iter = this->_awakes.erase(iter);

				if (awaker->awake(this, 1))
				{
					if (--this->_counter == 0)
						break;
				}
			}
		}

		void event_impl::reset()
		{
			scoped_lock<spinlock> lock_(this->_lock);

			this->_awakes.clear();
			this->_counter = 0;
		}

		bool event_impl::wait_(const event_awaker_ptr & awaker)
		{
			assert(awaker);

			scoped_lock<spinlock> lock_(this->_lock);

			if (this->_counter > 0)
			{
				if(awaker->awake(this, 1))
				{
					--this->_counter;
					return true;
				}
			}
			else
			{
				this->_awakes.push_back(awaker);
			}
			return false;
		}

	}

	event_t::event_t(intptr_t initial_counter_)
		: _event(std::make_shared<detail::event_impl>(initial_counter_))
	{
	}

	awaitable_t<bool> event_t::wait() const
	{
		awaitable_t<bool> awaitable;

		auto awaker = std::make_shared<detail::event_awaker>(
			[st = awaitable._state](detail::event_impl * e) -> bool
			{
				st->set_value(e ? true : false);
				return true;
			});
		_event->wait_(awaker);

		return awaitable;
	}

	awaitable_t<bool> event_t::wait_until_(const clock_type::time_point & tp) const
	{
		awaitable_t<bool> awaitable;

		auto awaker = std::make_shared<detail::event_awaker>(
			[st = awaitable._state](detail::event_impl * e) -> bool
			{
				st->set_value(e ? true : false);
				return true;
			});
		_event->wait_(awaker);
		
		g_scheduler.timer()->add(tp, 
			[awaker](bool bValue)
			{
				awaker->awake(nullptr, 1);
			});

		return awaitable;
	}

	struct wait_any_awaker
	{
		typedef state_t<intptr_t> state_type;
		counted_ptr<state_type> st;
		std::vector<detail::event_impl *> evts;

		wait_any_awaker(const counted_ptr<state_type> & st_, std::vector<detail::event_impl *> && evts_)
			: st(st_)
			, evts(std::forward<std::vector<detail::event_impl *>>(evts_))
		{}
		wait_any_awaker(const wait_any_awaker &) = delete;
		wait_any_awaker(wait_any_awaker &&) = default;

		bool operator()(detail::event_impl * e) const
		{
			if (e)
			{
				for (auto i = evts.begin(); i != evts.end(); ++i)
				{
					if (e == (*i))
					{
						st->set_value((intptr_t)(i - evts.begin()));
						return true;
					}
				}
			}
			else
			{
				st->set_value(-1);
			}

			return false;
		}
	};

	awaitable_t<intptr_t> event_t::wait_any_(std::vector<event_impl_ptr> && evts)
	{
		awaitable_t<intptr_t> awaitable;

		if (evts.size() <= 0)
		{
			awaitable._state->set_value(-1);
			return awaitable;
		}

		auto awaker = std::make_shared<detail::event_awaker>(
			[st = awaitable._state, evts](detail::event_impl * e) -> bool
			{
				if (e)
				{
					for (auto i = evts.begin(); i != evts.end(); ++i)
					{
						if (e == (*i).get())
						{
							st->set_value((intptr_t)(i - evts.begin()));
							return true;
						}
					}
				}
				else
				{
					st->set_value(-1);
				}

				return false;
			});

		for (auto e : evts)
		{
			e->wait_(awaker);
		}

		return awaitable;
	}
	
	awaitable_t<intptr_t> event_t::wait_any_until_(const clock_type::time_point & tp, std::vector<event_impl_ptr> && evts)
	{
		awaitable_t<intptr_t> awaitable;

		auto awaker = std::make_shared<detail::event_awaker>(
			[st = awaitable._state, evts](detail::event_impl * e) -> bool
			{
				if (e)
				{
					for (auto i = evts.begin(); i != evts.end(); ++i)
					{
						if (e == (*i).get())
						{
							st->set_value((intptr_t)(i - evts.begin()));
							return true;
						}
					}
				}
				else
				{
					st->set_value(-1);
				}

				return false;
			});

		for (auto e : evts)
		{
			e->wait_(awaker);
		}

		g_scheduler.timer()->add(tp, 
			[awaker](bool bValue)
			{
				awaker->awake(nullptr, 1);
			});

		return awaitable;
	}

	awaitable_t<bool> event_t::wait_all_(std::vector<event_impl_ptr> && evts)
	{
		awaitable_t<bool> awaitable;
		if (evts.size() <= 0)
		{
			awaitable._state->set_value(false);
			return awaitable;
		}

		auto awaker = std::make_shared<detail::event_awaker>(
			[st = awaitable._state](detail::event_impl * e) -> bool
			{
				st->set_value(e ? true : false);
				return true;
			},
			evts.size());

		for (auto e : evts)
		{
			e->wait_(awaker);
		}

		return awaitable;
	}


	struct event_t::wait_all_ctx
	{
		counted_ptr<state_t<bool>>		st;
		std::vector<event_impl_ptr>		evts;
		std::vector<event_impl_ptr>		evts_waited;
		timer_handler					th;
		spinlock						_lock;

		wait_all_ctx()
		{
#if RESUMEF_DEBUG_COUNTER
			++g_resumef_evtctx_count;
#endif
		}
		~wait_all_ctx()
		{
			th.stop();
#if RESUMEF_DEBUG_COUNTER
			--g_resumef_evtctx_count;
#endif
		}

		bool awake(detail::event_impl * eptr)
		{
			scoped_lock<spinlock> lock_(this->_lock);

			//如果st为nullptr，则说明之前已经返回过值了。本环境无效了。
			if (!st.get())
				return false;

			if (eptr)
			{
				//记录已经等到的事件
				evts_waited.emplace_back(eptr->shared_from_this());

				//已经等到的事件达到预期
				if (evts_waited.size() == evts.size())
				{
					evts_waited.clear();

					//返回true表示等待成功
					st->set_value(true);
					//丢弃st，以便于还有其它持有的ctx返回false
					st.reset();
					//取消定时器
					th.stop();
				}
			}
			else
			{
				//超时后，恢复已经等待的事件计数
				for (auto sptr : evts_waited)
				{
					sptr->signal();
				}
				evts_waited.clear();

				//返回true表示等待失败
				st->set_value(false);
				//丢弃st，以便于还有其它持有的ctx返回false
				st.reset();
				//定时器句柄已经无意义了
				th.reset();
			}

			return true;
		}
	};

	//等待所有的事件
	//超时后的行为应该表现为：
	//要么所有的事件计数减一，要么所有事件计数不动
	//则需要超时后，恢复已经等待的事件计数
	awaitable_t<bool> event_t::wait_all_until_(const clock_type::time_point & tp, std::vector<event_impl_ptr> && evts)
	{
		awaitable_t<bool> awaitable;
		if (evts.size() <= 0)
		{
			g_scheduler.timer()->add_handler(tp,
				[st = awaitable._state](bool bValue)
				{
					st->set_value(false);
				});
			return awaitable;
		}

		auto ctx = std::make_shared<wait_all_ctx>();
		ctx->st = awaitable._state;
		ctx->evts_waited.reserve(evts.size());
		ctx->evts = std::move(evts);
		ctx->th = std::move(g_scheduler.timer()->add_handler(tp,
			[ctx](bool bValue)
			{
				ctx->awake(nullptr);
			}));

		for (auto e : ctx->evts)
		{
			auto awaker = std::make_shared<detail::event_awaker>(
				[ctx](detail::event_impl * eptr) -> bool
				{
					return ctx->awake(eptr);
				});
			e->wait_(awaker);
		}


		return awaitable;
	}
}
