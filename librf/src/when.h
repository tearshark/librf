#pragma once

#include "_awaker.h"

namespace resumef
{
	namespace detail
	{
		struct when_impl;
		typedef _awaker<when_impl> when_awaker;
		typedef std::shared_ptr<when_awaker> when_awaker_ptr;

		struct when_impl : public std::enable_shared_from_this<when_impl>
		{
		private:
			//typedef spinlock lock_type;
			typedef std::recursive_mutex lock_type;

			when_awaker_ptr _awakes;
			intptr_t _counter;
			lock_type _lock;
		public:
			RF_API when_impl(intptr_t initial_counter_);

			RF_API void signal();
			RF_API void reset(intptr_t initial_counter_);

			//如果已经触发了awaker,则返回true
			RF_API bool wait_(const when_awaker_ptr & awaker);

			template<class callee_t, class dummy_t = std::enable_if<!std::is_same<std::remove_cv_t<callee_t>, event_awaker_ptr>::value>>
			auto wait(callee_t && awaker, dummy_t * dummy_ = nullptr)
			{
				return wait_(std::make_shared<event_awaker>(std::forward<callee_t>(awaker)));
			}

			when_impl(const when_impl &) = delete;
			when_impl(when_impl &&) = delete;
			when_impl & operator = (const when_impl &) = delete;
			when_impl & operator = (when_impl &&) = delete;
		};
		typedef std::shared_ptr<when_impl> when_impl_ptr;


		template<class _Fty>
		struct when_one_functor
		{
			when_impl_ptr _e;
			mutable future_t<_Fty> _f;

			when_one_functor(const detail::when_impl_ptr & e, future_t<_Fty> f)
				:_e(e)
				, _f(std::move(f))
			{}
			when_one_functor(when_one_functor &&) = default;

			inline future_vt operator ()() const
			{
				co_await _f;
				_e->signal();
			}
		};


		inline void when_one__(scheduler * s, const detail::when_impl_ptr & e)
		{
		}

		template<class _Fty, class... _Rest>
		inline void when_one__(scheduler * s, const detail::when_impl_ptr & e, future_t<_Fty> f, _Rest&&... rest)
		{
			(*s) + when_one_functor<_Fty>{e, std::move(f)};

			when_one__(s, e, std::forward<_Rest>(rest)...);
		}
	}

	template<class... _Fty>
	future_t<bool> when_count(size_t counter, scheduler * s, _Fty&&... f)
	{
		promise_t<bool> awaitable;

		detail::when_impl_ptr _event = std::make_shared<detail::when_impl>(counter);
		auto awaker = std::make_shared<detail::when_awaker>(
			[st = awaitable._state](detail::when_impl * e) -> bool
			{
				st->set_value(e ? true : false);
				return true;
			});
		_event->wait_(awaker);

		detail::when_one__(s, _event, std::forward<_Fty>(f)...);

		return awaitable.get_future();
	}

	template<class... _Fty>
	future_t<bool> when_all(scheduler * s, _Fty&&... f)
	{
		return when_count(sizeof...(_Fty), s, std::forward<_Fty>(f)...);
	}

	template<class... _Fty>
	future_t<bool> when_all(_Fty&&... f)
	{
		return when_count(sizeof...(_Fty), this_scheduler(), std::forward<_Fty>(f)...);
	}

	template<class... _Fty>
	future_t<bool> when_any(scheduler * s, _Fty&&... f)
	{
		static_assert(sizeof...(_Fty) > 0);
		return when_count(1, s, std::forward<_Fty>(f)...);
	}

	template<class... _Fty>
	future_t<bool> when_any(_Fty&&... f)
	{
		static_assert(sizeof...(_Fty) > 0);
		return when_count(1, this_scheduler(), std::forward<_Fty>(f)...);
	}
}
