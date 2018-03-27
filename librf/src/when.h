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

		using ignore_type = decltype(std::ignore);

		template<class _Ty>
		struct remove_future
		{
			using type = _Ty;
			using value_type = _Ty;
		};
		template<>
		struct remove_future<void>
		{
			using type = void;
			using value_type = ignore_type;
		};
		template<class _Ty>
		struct remove_future<future_t<_Ty> > : public remove_future<_Ty>
		{
		};
		template<class _Ty>
		struct remove_future<future_t<_Ty>&> : public remove_future<_Ty>
		{
		};
		template<class _Ty>
		struct remove_future<future_t<_Ty>&&> : public remove_future<_Ty>
		{
		};
		template<class _Ty>
		using remove_future_t = typename remove_future<_Ty>::type;

		template<class _Ty>
		using remove_future_vt = typename remove_future<_Ty>::value_type;


		template<class _Fty, class _Ty>
		struct when_one_functor_impl
		{
			using value_type = _Ty;
			using future_type = future_t<value_type>;

			when_impl_ptr _e;
			mutable future_type _f;
			mutable std::reference_wrapper<value_type> _val;

			when_one_functor_impl(const detail::when_impl_ptr & e, future_type f, value_type & v)
				: _e(e)
				, _f(std::move(f))
				, _val(v)
			{}
			when_one_functor_impl(when_one_functor_impl &&) = default;

			inline future_vt operator ()() const
			{
				_val.get() = co_await _f;
				_e->signal();
			}
		};
		template<class _Fty>
		struct when_one_functor_impl<_Fty, void>
		{
			using value_type = ignore_type;
			using future_type = future_t<void>;

			when_impl_ptr _e;
			mutable future_type _f;

			when_one_functor_impl(const detail::when_impl_ptr & e, future_type f, value_type & v)
				: _e(e)
				, _f(std::move(f))
			{}
			when_one_functor_impl(when_one_functor_impl &&) = default;

			inline future_vt operator ()() const
			{
				co_await _f;
				_e->signal();
			}
		};

		template<class _Fty>
		struct when_one_functor : public when_one_functor_impl<_Fty, remove_future_t<_Fty> >
		{
			using base_type = when_one_functor_impl<_Fty, remove_future_t<_Fty> >;
			using value_type = typename base_type::value_type;
			using future_type = typename base_type::future_type;

			when_one_functor(const detail::when_impl_ptr & e, future_type f, value_type & v)
				: when_one_functor_impl<_Fty, remove_future_t<_Fty> >(e, std::move(f), v)
			{}
			when_one_functor(when_one_functor &&) = default;
		};

		template<class... _Ty>
		size_t sizeof_tuple(const std::tuple<_Ty...> & val)
		{
			return sizeof...(_Ty);
		}
		template<class _Cont>
		size_t sizeof_tuple(const _Cont & val)
		{
			return val.typename size();
		}

		template<class _Tup, size_t _N>
		inline void when_all_one__(scheduler & s, const detail::when_impl_ptr & e, _Tup & t)
		{
		}

		template<class _Tup, size_t _N, class _Fty, class... _Rest>
		inline void when_all_one__(scheduler & s, const detail::when_impl_ptr & e, _Tup & t, _Fty f, _Rest&&... rest)
		{
			s + when_one_functor<_Fty>{e, std::move(f), std::get<_N>(t)};

			when_all_one__<_Tup, _N + 1, _Rest...>(s, e, t, std::forward<_Rest>(rest)...);
		}

		template<class _Val, class _Iter, typename _Fty = decltype(*std::declval<_Iter>())>
		inline void when_all_range__(scheduler & s, const detail::when_impl_ptr & e, std::vector<_Val> & t, _Iter begin, _Iter end)
		{
			using future_type = std::remove_reference_t<_Fty>;

			const auto _First = begin;
			for(; begin != end; ++begin)
				s + when_one_functor<future_type>{e, *begin, t[begin - _First]};
		}

		inline void when_any_one__(scheduler & s, const detail::when_impl_ptr & e)
		{
		}

		template<class _Fty, class... _Rest>
		inline void when_any_one__(scheduler & s, const detail::when_impl_ptr & e, future_t<_Fty> f, _Rest&&... rest)
		{
			s + when_one_functor<_Fty>{e, std::move(f)};

			when_any_one__(s, e, std::forward<_Rest>(rest)...);
		}

		template<class _Iter, typename _Fty = decltype(*std::declval<_Iter>())>
		inline void when_any_one__(scheduler & s, const detail::when_impl_ptr & e, _Iter begin, _Iter end)
		{
			using future_type = std::remove_reference_t<_Fty>;
			for (; begin != end; ++begin)
				s + when_one_functor<future_type>{e, *begin};
		}
	}

	template<class _Tup, class... _Fty>
	future_t<_Tup> when_all_count(const std::shared_ptr<_Tup> & vals, scheduler & s, _Fty&&... f)
	{
		promise_t<_Tup> awaitable;

		detail::when_impl_ptr _event = std::make_shared<detail::when_impl>(detail::sizeof_tuple(*vals));
		auto awaker = std::make_shared<detail::when_awaker>(
			[st = awaitable._state, vals](detail::when_impl * e) -> bool
			{
				if (e)
					st->set_value(*vals);
				else
					st->throw_exception(channel_exception{ error_code::not_ready });
				return true;
			});
		_event->wait_(awaker);

		detail::when_all_one__<_Tup, 0u, _Fty...>(s, _event, *vals, std::forward<_Fty>(f)...);

		return awaitable.get_future();
	}

	template<class _Tup, class _Iter>
	future_t<_Tup> when_all_range(const std::shared_ptr<_Tup> & vals, scheduler & s, _Iter begin, _Iter end)
	{
		promise_t<_Tup> awaitable;

		detail::when_impl_ptr _event = std::make_shared<detail::when_impl>(detail::sizeof_tuple(*vals));
		auto awaker = std::make_shared<detail::when_awaker>(
			[st = awaitable._state, vals](detail::when_impl * e) -> bool
			{
				if (e)
					st->set_value(*vals);
				else
					st->throw_exception(channel_exception{ error_code::not_ready });
				return true;
			});
		_event->wait_(awaker);

		detail::when_all_range__(s, _event, *vals, begin, end);

		return awaitable.get_future();
	}

	template<class... _Fty>
	future_t<bool> when_any_count(size_t counter, scheduler & s, _Fty&&... f)
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

		detail::when_any_one__(s, _event, std::forward<_Fty>(f)...);

		return awaitable.get_future();
	}


	template<class... _Fty>
	auto when_all(scheduler & s, _Fty&&... f) -> future_t<std::tuple<detail::remove_future_vt<_Fty>...> >
	{
		using tuple_type = std::tuple<detail::remove_future_vt<_Fty>...>;
		auto vals = std::make_shared<tuple_type>();

		return when_all_count(vals, s, std::forward<_Fty>(f)...);
	}
	template<class... _Fty>
	auto when_all(_Fty&&... f) -> future_t<std::tuple<detail::remove_future_vt<_Fty>...> >
	{
		using tuple_type = std::tuple<detail::remove_future_vt<_Fty>...>;
		auto vals = std::make_shared<tuple_type>();

		return when_all_count(vals, *this_scheduler(), std::forward<_Fty>(f)...);
	}
	template<class _Iter, typename _Fty = decltype(*std::declval<_Iter>())>
	auto when_all(_Iter begin, _Iter end) -> future_t<std::vector<detail::remove_future_vt<decltype(*std::declval<_Iter>())> > >
	{
		using value_type = detail::remove_future_vt<decltype(*std::declval<_Iter>())>;
		using vector_type = std::vector<value_type>;
		auto vals = std::make_shared<vector_type>(std::distance(begin, end));

		return when_all_range(vals, *this_scheduler(), begin, end);
	}





	template<class... _Fty>
	future_t<bool> when_any(scheduler & s, future_t<_Fty>&&... f)
	{
		static_assert(sizeof...(_Fty) > 0);
		return when_any_count(sizeof...(_Fty) ? 1 : 0, s, std::forward<future_t<_Fty>>(f)...);
	}
	template<class... _Fty>
	future_t<bool> when_any(future_t<_Fty>&&... f)
	{
		static_assert(sizeof...(_Fty) > 0);
		return when_any_count(sizeof...(_Fty) ? 1 : 0, *this_scheduler(), std::forward<future_t<_Fty>>(f)...);
	}
	template<class _Iter, typename = decltype(*std::declval<_Iter>())>
	future_t<bool> when_any(_Iter begin, _Iter end)
	{
		assert(std::distance(begin, end) > 0);	//???

		return when_any_count(std::distance(begin, end) ? 1 : 0, *this_scheduler(), begin, end);
	}
}
