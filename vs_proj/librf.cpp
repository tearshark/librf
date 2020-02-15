
#include "librf.h"
#include <experimental/resumable>
#include <experimental/generator>
#include <optional>

extern void resumable_main_yield_return();
extern void resumable_main_timer();
extern void resumable_main_suspend_always();
extern void resumable_main_sleep();
extern void resumable_main_routine();
extern void resumable_main_resumable();
extern void resumable_main_mutex();
extern void resumable_main_exception();
extern void resumable_main_event();
extern void resumable_main_event_timeout();
extern void resumable_main_dynamic_go();
extern void resumable_main_channel();
extern void resumable_main_cb();
extern void resumable_main_modern_cb();
extern void resumable_main_multi_thread();
extern void resumable_main_channel_mult_thread();
extern void resumable_main_when_all();

extern void resumable_main_benchmark_mem();
extern void resumable_main_benchmark_asio_server();
extern void resumable_main_benchmark_asio_client(intptr_t nNum);

namespace coro = std::experimental;

namespace librf2
{
	struct scheduler_t;

	template<class _Ty = void>
	struct future_t;

	template<class _Ty = void>
	struct promise_t;

	template<class _PromiseT>
	struct is_promise : std::false_type {};
	template<class _Ty>
	struct is_promise<promise_t<_Ty>> : std::true_type {};
	template<class _Ty>
	_INLINE_VAR constexpr bool is_promise_v = is_promise<_Ty>::value;

	struct state_base_t
	{
		scheduler_t* _scheduler = nullptr;
		coro::coroutine_handle<> _coro;
		std::exception_ptr _exception;

		virtual ~state_base_t() {}
		virtual bool has_value() const = 0;

		void resume()
		{
			auto handler = _coro;
			_coro = nullptr;
			handler();
		}
	};

	template<class _Ty = void>
	struct state_t : public state_base_t
	{
		using value_type = _Ty;

		std::optional<value_type> _value;
		virtual bool has_value() const override
		{
			return _value.has_value();
		}
	};

	template<>
	struct state_t<void> : public state_base_t
	{
		bool _has_value = false;
		virtual bool has_value() const override
		{
			return _has_value;
		}
	};

	struct scheduler_t
	{
		using state_sptr = std::shared_ptr<state_base_t>;
		using state_vector = std::vector<state_sptr>;
	private:
		state_vector	_runing_states;
	public:

		void add_initial(state_sptr sptr)
		{
			sptr->_scheduler = this;
			assert(sptr->_coro != nullptr);
			_runing_states.emplace_back(std::move(sptr));
		}

		void add_await(state_sptr sptr, coro::coroutine_handle<> handler)
		{
			sptr->_scheduler = this;
			sptr->_coro = handler;
			if (sptr->has_value() || sptr->_exception != nullptr)
				_runing_states.emplace_back(std::move(sptr));
		}

		void add_ready(state_sptr sptr)
		{
			assert(sptr->_scheduler == this);
			if (sptr->_coro != nullptr)
				_runing_states.emplace_back(std::move(sptr));
		}

		void run()
		{
			for (;;)
			{
				state_vector states = std::move(_runing_states);
				for (state_sptr& sptr : states)
					sptr->resume();
			}
		}
	};

	template<class _Ty>
	struct future_t
	{
		using value_type = _Ty;
		using state_type = state_t<value_type>;
		using promise_type = promise_t<value_type>;
		using future_type = future_t<value_type>;

		std::shared_ptr<state_type> _state;

		future_t(std::shared_ptr<state_type> _st)
			:_state(std::move(_st)) {}
		future_t(const future_t&) = default;
		future_t(future_t&&) = default;

		future_t& operator = (const future_t&) = default;
		future_t& operator = (future_t&&) = default;

		bool await_ready()
		{
			return _state->has_value();
		}

		template<class _PromiseT, typename = std::enable_if_t<is_promise_v<_PromiseT>>>
		void await_suspend(coro::coroutine_handle<_PromiseT> handler)
		{
			_PromiseT& promise = handler.promise();
			scheduler_t * sch = promise._state->_scheduler;
			sch->add_await(_state, handler);
		}

		value_type await_resume()
		{
			if (_state->_exception)
				std::rethrow_exception(std::move(_state->_exception));
			return std::move(_state->_value.value());
		}

		void resume() const
		{
			auto coro_handle = _state->_coro;
			_state->_coro = nullptr;
			coro_handle();
		}
	};

	struct suspend_on_initial
	{
		std::shared_ptr<state_base_t> _state;

		bool await_ready() noexcept
		{
			return false;
		}
		void await_suspend(coro::coroutine_handle<> handler) noexcept
		{
			_state->_coro = handler;
		}
		void await_resume() noexcept
		{
		}
	};

	template<class _Ty>
	struct promise_impl_t
	{
		using value_type = _Ty;
		using state_type = state_t<value_type>;
		using promise_type = promise_t<value_type>;
		using future_type = future_t<value_type>;

		std::shared_ptr<state_type> _state = std::make_shared<state_type>();

		promise_impl_t() {}
		promise_impl_t(const promise_impl_t&) = delete;
		promise_impl_t(promise_impl_t&&) = delete;

		promise_impl_t& operator = (const promise_impl_t&) = delete;
		promise_impl_t& operator = (promise_impl_t&&) = delete;

		auto initial_suspend() noexcept
		{
			return suspend_on_initial{ _state };
		}

		auto final_suspend() noexcept
		{
			return coro::suspend_never{};
		}

		void set_exception(std::exception_ptr e)
		{
			_state->_exception = std::move(e);
			if (_state->_scheduler != nullptr)
				_state->_scheduler->add_ready(_state);
		}

		future_t<value_type> get_return_object()
		{
			return { _state };
		}

		void cancellation_requested()
		{

		}
	};

	template<class _Ty>
	struct promise_t : public promise_impl_t<_Ty>
	{
		void return_value(value_type val)
		{
			_state->_value = std::move(val);
			if (_state->_scheduler != nullptr)
				_state->_scheduler->add_ready(_state);
		}

		void yield_value(value_type val)
		{
			_state->_value = std::move(val);
			if (_state->_scheduler != nullptr)
				_state->_scheduler->add_ready(_state);
		}
	};

	template<>
	struct promise_t<void> : public promise_impl_t<void>
	{
		void return_void()
		{
			_state->_has_value = true;
			if (_state->_scheduler != nullptr)
				_state->_scheduler->add_ready(_state);
		}

		void yield_value()
		{
			_state->_has_value = true;
			if (_state->_scheduler != nullptr)
				_state->_scheduler->add_ready(_state);
		}
	};

	template<class _Ty>
	struct awaitable_t
	{
		using value_type = _Ty;
		using state_type = state_t<value_type>;
		using future_type = future_t<value_type>;

	private:
		mutable std::shared_ptr<state_type> _state = std::make_shared<state_type>();
	public:
		awaitable_t() {}
		awaitable_t(const awaitable_t&) = default;
		awaitable_t(awaitable_t&&) = default;

		awaitable_t& operator = (const awaitable_t&) = default;
		awaitable_t& operator = (awaitable_t&&) = default;

		void set_value(value_type value) const
		{
			_state->_value = std::move(value);
			if (_state->_scheduler != nullptr)
				_state->_scheduler->add_ready(_state);
			_state = nullptr;
		}

		void set_exception(std::exception_ptr e)
		{
			_state->_exception = std::move(e);
			if (_state->_scheduler != nullptr)
				_state->_scheduler->add_ready(_state);
			_state = nullptr;
		}

		future_type get_future()
		{
			return future_type{ _state };
		}
	};

	template<>
	struct awaitable_t<void>
	{
		using value_type = void;
		using state_type = state_t<>;
		using future_type = future_t<>;

		mutable std::shared_ptr<state_type> _state = std::make_shared<state_type>();

		awaitable_t() {}
		awaitable_t(const awaitable_t&) = default;
		awaitable_t(awaitable_t&&) = default;

		awaitable_t& operator = (const awaitable_t&) = default;
		awaitable_t& operator = (awaitable_t&&) = default;

		void set_value() const
		{
			_state->_has_value = true;
			if (_state->_scheduler != nullptr)
				_state->_scheduler->add_ready(_state);
			_state = nullptr;
		}

		void set_exception(std::exception_ptr e)
		{
			_state->_exception = std::move(e);
			if (_state->_scheduler != nullptr)
				_state->_scheduler->add_ready(_state);
			_state = nullptr;
		}

		future_type get_future()
		{
			return future_type{ _state };
		}
	};
}

void async_get_long(int64_t val, std::function<void(int64_t)> cb)
{
	using namespace std::chrono;
	std::thread([val, cb = std::move(cb)]
		{
			std::this_thread::sleep_for(10s);
			cb(val * val);
		}).detach();
}

//这种情况下，没有生成 frame-context，因此，并没有promise_type被内嵌在frame-context里
librf2::future_t<int64_t> co_get_long(int64_t val)
{
	librf2::awaitable_t<int64_t > st;
	std::cout << "co_get_long@1" << std::endl;
	
	async_get_long(val, [st](int64_t value)
	{
		std::cout << "co_get_long@2" << std::endl;
		st.set_value(value);
	});

	std::cout << "co_get_long@3" << std::endl;
	return st.get_future();
}

//这种情况下，会生成对应的 frame-context，一个promise_type被内嵌在frame-context里
librf2::future_t<> test_librf2()
{
	auto f = co_await co_get_long(2);
	std::cout << f << std::endl;
}

int main(int argc, const char* argv[])
{
	librf2::scheduler_t sch;

	librf2::future_t<> ft = test_librf2();
	sch.add_initial(ft._state);
	sch.run();

	resumable_main_resumable();
	//resumable_main_benchmark_mem();
/*
	if (argc > 1)
		resumable_main_benchmark_asio_client(atoi(argv[1]));
	else
		resumable_main_benchmark_asio_server();
*/
	//return 0;

	resumable_main_when_all();
	resumable_main_multi_thread();
	resumable_main_yield_return();
	resumable_main_timer();
	resumable_main_suspend_always();
	resumable_main_sleep();
	resumable_main_routine();
	resumable_main_resumable();
	resumable_main_mutex();
	resumable_main_event();
	resumable_main_event_timeout();
	resumable_main_dynamic_go();
	resumable_main_channel();
	resumable_main_cb();
	resumable_main_exception();

	return 0;
}
