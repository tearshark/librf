
#include "future.h"
#include <memory>

#include "asio/detail/push_options.hpp"

namespace asio {

	template <typename Executor = executor>
	struct rf_task_t
	{
		ASIO_CONSTEXPR rf_task_t() {}
	};

#if defined(ASIO_HAS_CONSTEXPR) || defined(GENERATING_DOCUMENTATION)
	constexpr rf_task_t<> rf_task;
#elif defined(ASIO_MSVC)
	__declspec(selectany) rf_task_t<> rf_task;
#endif

	namespace librf {

		template <typename Executor, typename T>
		struct promise_handler_base
		{
		public:
			typedef T result_type;
			typedef resumef::state_t<result_type> state_type;

			promise_handler_base()
				: state_(resumef::make_counted<state_type>(true))
			{
			}

			resumef::counted_ptr<state_type> state_;
			promise_handler_base(promise_handler_base &&) = default;
			promise_handler_base(const promise_handler_base &) = default;
			promise_handler_base & operator = (promise_handler_base &&) = default;
			promise_handler_base & operator = (const promise_handler_base &) = default;
		};

		template <typename, typename...>
		struct promise_handler;

		template <typename Executor>
		struct promise_handler<Executor, void> : public promise_handler_base<Executor, void>
		{
			using promise_handler_base<Executor, void>::promise_handler_base;

			void operator()() const
			{
				this->state_->set_value();
			}
		};

		template <typename Executor>
		struct promise_handler<Executor, asio::error_code> : public promise_handler_base<Executor, void>
		{
			using promise_handler_base<Executor, void>::promise_handler_base;

			void operator()(const asio::error_code& ec) const
			{
				if (!ec)
					this->state_->set_value();
				else
					this->state_->set_exception(std::make_exception_ptr(asio::system_error(ec)));
			}
		};

		template <typename Executor>
		struct promise_handler<Executor, std::exception_ptr> : public promise_handler_base<Executor, void>
		{
			using promise_handler_base<Executor, void>::promise_handler_base;

			void operator()(std::exception_ptr ex) const
			{
				if (!ex)
					this->state_->set_value();
				else
					this->state_->set_exception(ex);
			}
		};



		template <typename Executor, typename T>
		struct promise_handler<Executor, T> : public promise_handler_base<Executor, T>
		{
			using promise_handler_base<Executor, T>::promise_handler_base;

			template <typename Arg>
			void operator()(Arg&& arg) const
			{
				this->state_->set_value(std::forward<Arg>(arg));
			}
		};

		template <typename Executor, typename T>
		struct promise_handler<Executor, asio::error_code, T> : public promise_handler_base<Executor, T>
		{
			using promise_handler_base<Executor, T>::promise_handler_base;

			template <typename Arg>
			void operator()(const asio::error_code& ec, Arg&& arg) const
			{
				if (!ec)
					this->state_->set_value(std::forward<Arg>(arg));
				else
					this->state_->set_exception(std::make_exception_ptr(asio::system_error(ec)));
			}
		};

		template <typename Executor, typename T>
		struct promise_handler<Executor, std::exception_ptr, T> : public promise_handler_base<Executor, T>
		{
			using promise_handler_base<Executor, T>::promise_handler_base;

			template <typename Arg>
			void operator()(std::exception_ptr ex, Arg&& arg) const
			{
				if (!ex)
					this->state_->set_value(std::forward<Arg>(arg));
				else
					this->state_->set_exception(ex);
			}
		};



		template <typename Executor, typename... Ts>
		struct promise_handler : public promise_handler_base<Executor, std::tuple<Ts...>>
		{
			using promise_handler_base<Executor, std::tuple<Ts...>>::promise_handler_base;

			template <typename... Args>
			void operator()(Args&&... args) const
			{
				this->state_->set_value(std::make_tuple(std::forward<Args>(args)...));
			}
		};

		template <typename Executor, typename... Ts>
		struct promise_handler<Executor, asio::error_code, Ts...> : public promise_handler_base<Executor, std::tuple<Ts...>>
		{
			using promise_handler_base<Executor, std::tuple<Ts...>>::promise_handler_base;

			template <typename... Args>
			void operator()(const asio::error_code& ec, Args&&... args) const
			{
				if (!ec)
					this->state_->set_value(std::make_tuple(std::forward<Args>(args)...));
				else
					this->state_->set_exception(std::make_exception_ptr(asio::system_error(ec)));
			}
		};

		template <typename Executor, typename... Ts>
		struct promise_handler<Executor, std::exception_ptr, Ts...> : public promise_handler_base<Executor, std::tuple<Ts...>>
		{
			using promise_handler_base<Executor, std::tuple<Ts...>>::promise_handler_base;

			template <typename... Args>
			void operator()(std::exception_ptr ex, Args&&... args) const
			{
				if (!ex)
					this->state_->set_value(std::make_tuple(std::forward<Args>(args)...));
				else
					this->state_->set_exception(ex);
			}
		};

	}  // namespace librf

	template <typename Executor, typename R, typename... Args>
	class async_result<rf_task_t<Executor>, R(Args...)>
	{
	public:
		typedef librf::promise_handler<Executor, Args...> handler_type;
		typedef typename handler_type::result_type result_type;
		typedef resumef::future_t<result_type> return_type;

		template <typename Initiation, typename... InitArgs>
		static return_type initiate(ASIO_MOVE_ARG(Initiation) initiation,
			rf_task_t<Executor>, ASIO_MOVE_ARG(InitArgs)... args)
		{
			handler_type handler{};
			return_type future{ handler.state_ };

			std::move(initiation)(std::move(handler), std::move(args)...);
			return std::move(future);
		}
	};

}  // namespace asio

#include "asio/detail/pop_options.hpp"
