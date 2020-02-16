#pragma once
#if RESUMEF_USE_BOOST_ANY
#include <boost/any.hpp>
namespace resumef
{
	using any_t = boost::any;
	using boost::any_cast;
}
#else
#include <any>
namespace resumef
{
	using any_t = std::any;
	using std::any_cast;
}
#endif

#include "_awaker.h"
#include "promise.h"
#include "promise.inl"


//纠结过when_any的返回值，是选用index + std::any，还是选用std::variant<>。最终选择了std::any。
//std::variant<>存在第一个元素不能默认构造的问题，需要使用std::monostate来占位，导致下标不是从0开始。
//而且，std::variant<>里面存在类型重复的问题，好几个操作都是病态的
//最最重要的，要统一ranged when_any的返回值，还得做一个运行时通过下标设置std::variant<>的东西
//std::any除了内存布局不太理想，其他方面几乎没缺点（在此应用下）

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
				return wait_(std::make_shared<when_awaker>(std::forward<callee_t>(awaker)));
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
			using type = std::remove_reference_t<_Ty>;
			using value_type = type;
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
		using remove_future_vt = typename remove_future<_Ty>::value_type;

		template<class _Fty, class _Ty>
		struct when_all_functor
		{
			using value_type = _Ty;
			using future_type = _Fty;

			when_impl_ptr _e;
			mutable future_type _f;
			mutable std::reference_wrapper<value_type> _val;

			when_all_functor(const detail::when_impl_ptr & e, future_type f, value_type & v)
				: _e(e)
				, _f(std::move(f))
				, _val(v)
			{}
			when_all_functor(when_all_functor &&) noexcept = default;
			when_all_functor & operator = (const when_all_functor &) = default;
			when_all_functor & operator = (when_all_functor &&) = default;

			inline future_t<> operator ()() const
			{
				_val.get() = co_await _f;
				_e->signal();
			}
		};

		template<class _Ty>
		struct when_all_functor<future_t<>, _Ty>
		{
			using value_type = _Ty;
			using future_type = future_t<>;

			when_impl_ptr _e;
			mutable future_type _f;
			mutable std::reference_wrapper<value_type> _val;

			when_all_functor(const detail::when_impl_ptr & e, future_type f, value_type & v)
				: _e(e)
				, _f(std::move(f))
				, _val(v)
			{}
			when_all_functor(when_all_functor &&) noexcept = default;
			when_all_functor & operator = (const when_all_functor &) = default;
			when_all_functor & operator = (when_all_functor &&) = default;

			inline future_t<> operator ()() const
			{
				co_await _f;
				_val.get() = std::ignore;
				_e->signal();
			}
		};

		template<class _Tup, size_t _Idx>
		inline void when_all_one__(scheduler_t & , const when_impl_ptr & , _Tup & )
		{
		}

		template<class _Tup, size_t _Idx, class _Fty, class... _Rest>
		inline void when_all_one__(scheduler_t& s, const when_impl_ptr & e, _Tup & t, _Fty f, _Rest&&... rest)
		{
			s + when_all_functor<_Fty, std::tuple_element_t<_Idx, _Tup> >{e, std::move(f), std::get<_Idx>(t)};

			when_all_one__<_Tup, _Idx + 1, _Rest...>(s, e, t, std::forward<_Rest>(rest)...);
		}

		template<class _Val, class _Iter, typename _Fty = decltype(*std::declval<_Iter>())>
		inline void when_all_range__(scheduler_t& s, const when_impl_ptr & e, std::vector<_Val> & t, _Iter begin, _Iter end)
		{
			using future_type = std::remove_reference_t<_Fty>;

			const auto _First = begin;
			for(; begin != end; ++begin)
				s + when_all_functor<future_type, _Val>{e, *begin, t[begin - _First]};
		}



		template<class _Tup, class... _Fty>
		future_t<_Tup> when_all_count(size_t count, const std::shared_ptr<_Tup> & vals, scheduler_t & s, _Fty&&... f)
		{
			awaitable_t<_Tup> awaitable;

			when_impl_ptr _event = std::make_shared<when_impl>(count);
			auto awaker = std::make_shared<when_awaker>(
				[st = awaitable._state, vals](when_impl * e) -> bool
				{
					if (e)
						st->set_value(*vals);
					else
						st->throw_exception(channel_exception{ error_code::not_ready });
					return true;
				});
			_event->wait_(awaker);

			when_all_one__<_Tup, 0u, _Fty...>(s, _event, *vals, std::forward<_Fty>(f)...);

			return awaitable.get_future();
		}

		template<class _Tup, class _Iter>
		future_t<_Tup> when_all_range(size_t count, const std::shared_ptr<_Tup> & vals, scheduler_t& s, _Iter begin, _Iter end)
		{
			awaitable_t<_Tup> awaitable;

			when_impl_ptr _event = std::make_shared<when_impl>(count);
			auto awaker = std::make_shared<when_awaker>(
				[st = awaitable._state, vals](when_impl * e) -> bool
				{
					if (e)
						st->set_value(*vals);
					else
						st->throw_exception(channel_exception{ error_code::not_ready });
					return true;
				});
			_event->wait_(awaker);

			when_all_range__(s, _event, *vals, begin, end);

			return awaitable.get_future();
		}

		using when_any_pair = std::pair<intptr_t, any_t>;
		using when_any_result_ptr = std::shared_ptr<when_any_pair>;

		template<class _Fty>
		struct when_any_functor
		{
			using value_type = when_any_pair;
			using future_type = _Fty;

			when_impl_ptr _e;
			mutable future_type _f;
			mutable when_any_result_ptr _val;
			intptr_t _Idx;

			when_any_functor(const when_impl_ptr & e, future_type f, const when_any_result_ptr & v, intptr_t idx)
				: _e(e)
				, _f(std::move(f))
				, _val(v)
				, _Idx(idx)
			{
				assert(idx >= 0);
			}
			when_any_functor(when_any_functor &&) noexcept = default;
			when_any_functor & operator = (const when_any_functor &) = default;
			when_any_functor & operator = (when_any_functor &&) = default;

			inline future_t<> operator ()() const
			{
				if (_val->first < 0)
				{
					auto tval = co_await _f;
					if (_val->first < 0)
					{
						_val->first = _Idx;
						_val->second = std::move(tval);
						_e->signal();
					}
				}
				else
				{
					co_await _f;
				}
			}
		};

		template<>
		struct when_any_functor<future_t<>>
		{
			using value_type = when_any_pair;
			using future_type = future_t<>;

			when_impl_ptr _e;
			mutable future_type _f;
			mutable when_any_result_ptr _val;
			intptr_t _Idx;

			when_any_functor(const when_impl_ptr & e, future_type f, const when_any_result_ptr & v, intptr_t idx)
				: _e(e)
				, _f(std::move(f))
				, _val(v)
				, _Idx(idx)
			{
				assert(idx >= 0);
			}
			when_any_functor(when_any_functor &&) noexcept = default;
			when_any_functor & operator = (const when_any_functor &) = default;
			when_any_functor & operator = (when_any_functor &&) = default;

			inline future_t<> operator ()() const
			{
				if (_val->first < 0)
				{
					co_await _f;
					if (_val->first < 0)
					{
						_val->first = _Idx;
						_e->signal();
					}
				}
				else
				{
					co_await _f;
				}
			}
		};

		template<intptr_t _Idx>
		inline void when_any_one__(scheduler_t & , const when_impl_ptr & , const when_any_result_ptr & )
		{
		}

		template<intptr_t _Idx, class _Fty, class... _Rest>
		inline void when_any_one__(scheduler_t & s, const when_impl_ptr & e, const when_any_result_ptr & t, _Fty f, _Rest&&... rest)
		{
			s + when_any_functor<_Fty>{e, std::move(f), t, _Idx};

			when_any_one__<_Idx + 1, _Rest...>(s, e, t, std::forward<_Rest>(rest)...);
		}

		template<class... _Fty>
		future_t<when_any_pair> when_any_count(size_t count, const when_any_result_ptr & val_ptr, scheduler_t & s, _Fty&&... f)
		{
			awaitable_t<when_any_pair> awaitable;

			when_impl_ptr _event = std::make_shared<when_impl>(count);
			auto awaker = std::make_shared<when_awaker>(
				[st = awaitable._state, val_ptr](when_impl * e) -> bool
				{
					if (e)
						st->set_value(*val_ptr);
					else
						st->throw_exception(channel_exception{ error_code::not_ready });
					return true;
				});
			_event->wait_(awaker);

			when_any_one__<0u, _Fty...>(s, _event, val_ptr, std::forward<_Fty>(f)...);

			return awaitable.get_future();
		}
	

		template<class _Iter, typename _Fty = decltype(*std::declval<_Iter>())>
		inline void when_any_range__(scheduler_t & s, const when_impl_ptr & e, const when_any_result_ptr & t, _Iter begin, _Iter end)
		{
			using future_type = std::remove_reference_t<_Fty>;

			const auto _First = begin;
			for (; begin != end; ++begin)
				s + when_any_functor<future_type>{e, *begin, t, begin - _First};
		}

		template<class _Iter>
		future_t<when_any_pair> when_any_range(size_t count, const when_any_result_ptr & val_ptr, scheduler_t & s, _Iter begin, _Iter end)
		{
			awaitable_t<when_any_pair> awaitable;

			when_impl_ptr _event = std::make_shared<when_impl>(count);
			auto awaker = std::make_shared<when_awaker>(
				[st = awaitable._state, val_ptr](when_impl * e) -> bool
				{
					if (e)
						st->set_value(*val_ptr);
					else
						st->throw_exception(channel_exception{ error_code::not_ready });
					return true;
				});
			_event->wait_(awaker);

			when_any_range__(s, _event, val_ptr, begin, end);

			return awaitable.get_future();
		}
	}

	template<class... _Fty>
	auto when_all(scheduler_t & s, _Fty&&... f) -> future_t<std::tuple<detail::remove_future_vt<_Fty>...> >
	{
		using tuple_type = std::tuple<detail::remove_future_vt<_Fty>...>;
		auto vals = std::make_shared<tuple_type>();

		return detail::when_all_count(sizeof...(_Fty), vals, s, std::forward<_Fty>(f)...);
	}
	template<class... _Fty>
	auto when_all(_Fty&&... f) -> future_t<std::tuple<detail::remove_future_vt<_Fty>...> >
	{
		return when_all(*this_scheduler(), std::forward<_Fty>(f)...);
	}
	template<class _Iter, typename _Fty = decltype(*std::declval<_Iter>())>
	auto when_all(scheduler_t & s, _Iter begin, _Iter end) -> future_t<std::vector<detail::remove_future_vt<_Fty> > >
	{
		using value_type = detail::remove_future_vt<_Fty>;
		using vector_type = std::vector<value_type>;
		auto vals = std::make_shared<vector_type>(std::distance(begin, end));

		return detail::when_all_range(vals->size(), vals, s, begin, end);
	}
	template<class _Iter, typename _Fty = decltype(*std::declval<_Iter>())>
	auto when_all(_Iter begin, _Iter end) -> future_t<std::vector<detail::remove_future_vt<_Fty> > >
	{
		return when_all(*this_scheduler(), begin, end);
	}




	template<class... _Fty>
	auto when_any(scheduler_t & s, _Fty&&... f) -> future_t<detail::when_any_pair>
	{
		auto vals = std::make_shared<detail::when_any_pair>(-1, any_t{});

		return detail::when_any_count(sizeof...(_Fty) ? 1 : 0, vals, s, std::forward<_Fty>(f)...);
	}
	template<class... _Fty>
	auto when_any(_Fty&&... f) -> future_t<detail::when_any_pair>
	{
		return when_any(*this_scheduler(), std::forward<_Fty>(f)...);
	}
	template<class _Iter, typename _Fty = decltype(*std::declval<_Iter>())>
	auto when_any(scheduler_t & s, _Iter begin, _Iter end) -> future_t<detail::when_any_pair>
	{
		auto vals = std::make_shared<detail::when_any_pair>(-1, any_t{});

		return detail::when_any_range((begin != end) ? 1 : 0, vals, s, begin, end);
	}
	template<class _Iter, typename _Fty = decltype(*std::declval<_Iter>())>
	auto when_any(_Iter begin, _Iter end) -> future_t<detail::when_any_pair>
	{
		return when_any(*this_scheduler(), begin, end);
	}
}
