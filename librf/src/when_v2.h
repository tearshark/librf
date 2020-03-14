#pragma once

RESUMEF_NS
{
	using any_t = std::any;
	using std::any_cast;
}

//纠结过when_any的返回值，是选用index + std::any，还是选用std::variant<>。最终选择了std::any。
//std::variant<>存在第一个元素不能默认构造的问题，需要使用std::monostate来占位，导致下标不是从0开始。
//而且，std::variant<>里面存在类型重复的问题，好几个操作都是病态的
//最最重要的，要统一ranged when_any的返回值，还得做一个运行时通过下标设置std::variant<>的东西
//std::any除了内存布局不太理想，其他方面几乎没缺点（在此应用下）

RESUMEF_NS
{
	namespace detail
	{
		struct state_when_t : public state_base_t
		{
			state_when_t(intptr_t counter_);

			virtual void resume() override;
			virtual bool has_handler() const  noexcept override;

			void on_cancel() noexcept;
			bool on_notify_one();
			bool on_timeout();

			//将自己加入到通知链表里
			template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
			scheduler_t* on_await_suspend(coroutine_handle<_PromiseT> handler) noexcept
			{
				_PromiseT& promise = handler.promise();
				auto* parent_state = promise.get_state();
				scheduler_t* sch = parent_state->get_scheduler();

				this->_scheduler = sch;
				this->_coro = handler;

				return sch;
			}

			typedef spinlock lock_type;
			lock_type _lock;
			std::atomic<intptr_t> _counter;
		};

		template<class _Ty>
		struct [[nodiscard]] when_future_t
		{
			using value_type = _Ty;
			using state_type = detail::state_when_t;
			using promise_type = promise_t<value_type>;
			using future_type = when_future_t<value_type>;
			using lock_type = typename state_type::lock_type;

			counted_ptr<state_type> _state;
			std::shared_ptr<value_type> _values;

			when_future_t(intptr_t count_) noexcept
				: _state(new state_type(count_))
				, _values(std::make_shared<value_type>())
			{
			}

			bool await_ready() noexcept
			{
				return _state->_counter.load(std::memory_order_relaxed) == 0;
			}

			template<class _PromiseT, typename = std::enable_if_t<traits::is_promise_v<_PromiseT>>>
			void await_suspend(coroutine_handle<_PromiseT> handler)
			{
				_state->on_await_suspend(handler);
			}

			value_type await_resume() noexcept(std::is_nothrow_move_constructible_v<value_type>)
			{
				return std::move(*_values);
			}
		};


		using ignore_type = std::remove_const_t<decltype(std::ignore)>;

		template<class _Ty>
		struct convert_void_2_ignore
		{
			using type = std::remove_reference_t<_Ty>;
			using value_type = type;
		};
		template<>
		struct convert_void_2_ignore<void>
		{
			using type = void;
			using value_type = ignore_type;
		};

		template<class _Ty, bool = traits::is_callable_v<_Ty>>
		struct awaitor_result_impl
		{
			using value_type = typename convert_void_2_ignore<
				typename traits::awaitor_traits<_Ty>::value_type
			>::value_type;
		};
		template<class _Ty>
		struct awaitor_result_impl<_Ty, true> : awaitor_result_impl<decltype(std::declval<_Ty>()()), false> {};
		template<_WhenTaskT _Ty>
		using awaitor_result_t = typename awaitor_result_impl<std::remove_reference_t<_Ty>>::value_type;


		template<class _Ty>
		struct is_when_task : std::bool_constant<traits::is_awaitor_v<_Ty> || traits::is_callable_v<_Ty>> {};
		template<class _Ty>
		constexpr bool is_when_task_v = is_when_task<_Ty>::value;

		template<class _Ty, class _Task = decltype(*std::declval<_Ty>())>
		constexpr bool is_when_task_iter_v = traits::is_iterator_v<_Ty> && is_when_task_v<_Task>;

		template<_WhenTaskT _Awaitable>
		decltype(auto) when_real_awaitor(_Awaitable&& awaitor)
		{
			if constexpr (traits::is_callable_v<_Awaitable>)
				return awaitor();
			else
				return std::forward<_Awaitable>(awaitor);
		}

		template<_WhenTaskT _Awaitable, class _Ty>
		future_t<> when_all_connector(state_when_t* state, _Awaitable task, _Ty& value)
		{
			decltype(auto) awaitor = when_real_awaitor(task);

			if constexpr(std::is_same_v<_Ty, ignore_type>)
				co_await awaitor;
			else
				value = co_await awaitor;
			state->on_notify_one();
		};

		template<class _Tup, size_t _Idx>
		inline void when_all_one__(scheduler_t& , state_when_t*, _Tup& )
		{
		}

		template<class _Tup, size_t _Idx, _WhenTaskT _Awaitable, _WhenTaskT... _Rest>
		inline void when_all_one__(scheduler_t& sch, state_when_t* state, _Tup& values, _Awaitable&& awaitable, _Rest&&... rest)
		{
			sch + when_all_connector(state, std::forward<_Awaitable>(awaitable), std::get<_Idx>(values));

			when_all_one__<_Tup, _Idx + 1, _Rest...>(sch, state, values, std::forward<_Rest>(rest)...);
		}

		template<class _Val, _WhenIterT _Iter>
		inline void when_all_range__(scheduler_t& sch, state_when_t* state, std::vector<_Val> & values, _Iter begin, _Iter end)
		{
			intptr_t _Idx = 0;
			for (; begin != end; ++begin, ++_Idx)
			{
				sch + when_all_connector(state, std::move(*begin), values[_Idx]);
			}
		}

//-----------------------------------------------------------------------------------------------------------------------------------------

		using when_any_pair = std::pair<intptr_t, any_t>;
		using when_any_pair_ptr = std::shared_ptr<when_any_pair>;

		template<_WhenTaskT _Awaitable>
		future_t<> when_any_connector(counted_ptr<state_when_t> state, _Awaitable task, when_any_pair_ptr value, intptr_t idx)
		{
			assert(idx >= 0);

			decltype(auto) awaitor = when_real_awaitor(task);

			using value_type = awaitor_result_t<decltype(awaitor)>;

			if constexpr (std::is_same_v<value_type, ignore_type>)
			{
				co_await awaitor;

				intptr_t oldValue = -1;
				if (reinterpret_cast<std::atomic<intptr_t>&>(value->first).compare_exchange_strong(oldValue, idx))
				{
					state->on_notify_one();
				}
			}
			else
			{
				decltype(auto) result = co_await awaitor;

				intptr_t oldValue = -1;
				if (reinterpret_cast<std::atomic<intptr_t>&>(value->first).compare_exchange_strong(oldValue, idx))
				{
					value->second = std::move(result);

					state->on_notify_one();
				}
			}
		};

		inline void when_any_one__(scheduler_t&, state_when_t*, when_any_pair_ptr, intptr_t)
		{
		}

		template<_WhenTaskT _Awaitable, _WhenTaskT... _Rest>
		inline void when_any_one__(scheduler_t& sch, state_when_t* state, when_any_pair_ptr value, intptr_t _Idx, _Awaitable&& awaitable, _Rest&&... rest)
		{
			sch + when_any_connector(state, std::forward<_Awaitable>(awaitable), value, _Idx);

			when_any_one__(sch, state, value, _Idx + 1, std::forward<_Rest>(rest)...);
		}

		template<_WhenIterT _Iter>
		inline void when_any_range__(scheduler_t& sch, state_when_t* state, when_any_pair_ptr value, _Iter begin, _Iter end)
		{
			intptr_t _Idx = 0;
			for (; begin != end; ++begin, ++_Idx)
			{
				sch + when_any_connector(state, std::move(*begin), value, static_cast<intptr_t>(_Idx));
			}
		}
	}

inline namespace when_v2
{
	template<_WhenTaskT... _Awaitable
		COMMA_RESUMEF_ENABLE_IF(std::conjunction_v<detail::is_when_task<_Awaitable>...>)
	>
	auto when_all(scheduler_t& sch, _Awaitable&&... args)
		-> detail::when_future_t<std::tuple<detail::awaitor_result_t<_Awaitable>...> >
	{
		using tuple_type = std::tuple<detail::awaitor_result_t<_Awaitable>...>;

		detail::when_future_t<tuple_type> awaitor{ sizeof...(_Awaitable) };
		detail::when_all_one__<tuple_type, 0u, _Awaitable...>(sch, awaitor._state.get(), *awaitor._values, std::forward<_Awaitable>(args)...);

		return awaitor;
	}

	template<_WhenIterT _Iter
		COMMA_RESUMEF_ENABLE_IF(detail::is_when_task_iter_v<_Iter>)
	>
	auto when_all(scheduler_t& sch, _Iter begin, _Iter end)
		-> detail::when_future_t<std::vector<detail::awaitor_result_t<decltype(*std::declval<_Iter>())> > >
	{
		using value_type = detail::awaitor_result_t<decltype(*std::declval<_Iter>())>;
		using vector_type = std::vector<value_type>;

		detail::when_future_t<vector_type> awaitor{ std::distance(begin, end) };
		awaitor._values->resize(end - begin);
		when_all_range__(sch, awaitor._state.get(), *awaitor._values, begin, end);

		return awaitor;
	}

	template<_ContainerT _Cont
		COMMA_RESUMEF_ENABLE_IF(traits::is_container_v<_Cont>)
	>
	decltype(auto) when_all(scheduler_t& sch, _Cont& cont)
	{
		return when_all(sch, std::begin(cont), std::end(cont));
	}

	template<_WhenTaskT... _Awaitable
		COMMA_RESUMEF_ENABLE_IF(std::conjunction_v<detail::is_when_task<_Awaitable>...>)
	>
	auto when_all(_Awaitable&&... awaitor)
		-> future_t<std::tuple<detail::awaitor_result_t<_Awaitable>...>>
	{
		co_return co_await when_all(*current_scheduler(), std::forward<_Awaitable>(awaitor)...);
	}

	template<_WhenIterT _Iter
		COMMA_RESUMEF_ENABLE_IF(detail::is_when_task_iter_v<_Iter>)
	>
	auto when_all(_Iter begin, _Iter end)
		-> future_t<std::vector<detail::awaitor_result_t<decltype(*std::declval<_Iter>())>>>
	{
		co_return co_await when_all(*current_scheduler(), begin, end);
	}

	template<_ContainerT _Cont
		COMMA_RESUMEF_ENABLE_IF(traits::is_container_v<_Cont>)
	>
	decltype(auto) when_all(_Cont& cont)
	{
		return when_all(std::begin(cont), std::end(cont));
	}



	template<_WhenTaskT... _Awaitable
		COMMA_RESUMEF_ENABLE_IF(std::conjunction_v<detail::is_when_task<_Awaitable>...>)
	>
	auto when_any(scheduler_t& sch, _Awaitable&&... args)
		-> detail::when_future_t<detail::when_any_pair>
	{
		detail::when_future_t<detail::when_any_pair> awaitor{ sizeof...(_Awaitable) > 0 ? 1 : 0 };
		awaitor._values->first = -1;
		detail::when_any_one__(sch, awaitor._state.get(), awaitor._values, 0, std::forward<_Awaitable>(args)...);

		return awaitor;
	}

	template<_WhenIterT _Iter
		COMMA_RESUMEF_ENABLE_IF(detail::is_when_task_iter_v<_Iter>)
	>
	auto when_any(scheduler_t& sch, _Iter begin, _Iter end)
		-> detail::when_future_t<detail::when_any_pair>
	{
		detail::when_future_t<detail::when_any_pair> awaitor{ begin == end ? 0 : 1 };
		awaitor._values->first = -1;
		detail::when_any_range__<_Iter>(sch, awaitor._state.get(), awaitor._values, begin, end);

		return awaitor;
	}

	template<_ContainerT _Cont
		COMMA_RESUMEF_ENABLE_IF(traits::is_container_v<_Cont>)
	>
	decltype(auto) when_any(scheduler_t& sch, _Cont& cont)
	{
		return when_any(sch, std::begin(cont), std::end(cont));
	}

	template<_WhenTaskT... _Awaitable
		COMMA_RESUMEF_ENABLE_IF(std::conjunction_v<detail::is_when_task<_Awaitable>...>)
	>
	auto when_any(_Awaitable&&... awaitor) 
		-> future_t<detail::when_any_pair>
	{
		co_return co_await when_any(*current_scheduler(), std::forward<_Awaitable>(awaitor)...);
	}

	template<_WhenIterT _Iter
		COMMA_RESUMEF_ENABLE_IF(detail::is_when_task_iter_v<_Iter>)
	>
	auto when_any(_Iter begin, _Iter end) 
		-> future_t<detail::when_any_pair>
	{
		co_return co_await when_any(*current_scheduler(), begin, end);
	}

	template<_ContainerT _Cont
		COMMA_RESUMEF_ENABLE_IF(traits::is_container_v<_Cont>)
	>
	decltype(auto) when_any(_Cont& cont)
	{
		return when_any(std::begin(cont), std::end(cont));
	}

}
}
