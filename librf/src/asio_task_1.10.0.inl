
#pragma once

#include "future.h"
#include <memory>

#include "asio/detail/push_options.hpp"

namespace asio {

	template<typename Allocator = std::allocator<int> >
	class rf_task_t
	{
	public:
		typedef Allocator allocator_type;
		constexpr rf_task_t() {}
		explicit rf_task_t(const Allocator& allocator) : allocator_(allocator) {}

		template<typename OtherAllocator>
		rf_task_t<OtherAllocator> operator[](const OtherAllocator& allocator) const {
			return rf_task_t<OtherAllocator>(allocator);
		}

		allocator_type get_allocator() const { return allocator_; }
	private:
		Allocator allocator_;
	};

	//constexpr rf_task_t<> rf_task;
#pragma warning(push)
#pragma warning(disable : 4592)
	__declspec(selectany) rf_task_t<> rf_task;
#pragma warning(pop)

	namespace detail {

		template<typename T>
		class promise_handler
		{
		public:
			using result_type_t = T;
			using state_type = resumef::state_t<result_type_t>;

			template<typename Allocator>
			promise_handler(const rf_task_t<Allocator> &)
				: state_(resumef::make_counted<state_type>())
			{
			}

			void operator()(T t) const
			{
				state_->set_value(std::move(t));
			}

			void operator()(const asio::error_code& ec, T t) const
			{
				if (!ec)
				{
					state_->set_value(std::move(t));
				}
				else
				{
					state_->set_exception(std::make_exception_ptr(asio::system_error(ec)));
				}
			}

			resumef::counted_ptr<state_type> state_;
		};

		template<>
		class promise_handler<void>
		{
		public:
			using result_type_t = void;
			using state_type = resumef::state_t<result_type_t>;

			template<typename Allocator>
			promise_handler(const rf_task_t<Allocator> &)
				: state_(resumef::make_counted<state_type>())
			{
			}

			void operator()() const
			{
				state_->set_value();
			}

			void operator()(const asio::error_code& ec) const
			{
				if (!ec)
				{
					state_->set_value();
				}
				else
				{
					state_->set_exception(std::make_exception_ptr(asio::system_error(ec)));
				}
			}

			resumef::counted_ptr<state_type> state_;
		};

	}  // namespace detail

	template<typename T>
	class async_result<detail::promise_handler<T> >
	{
	public:
		typedef resumef::future_t<T> type;

		explicit async_result(detail::promise_handler<T> & h)
			: task_(std::move(h.state_))
		{ }

		resumef::future_t<T> get() { return std::move(task_); }
	private:
		resumef::future_t<T> task_;
	};

	// Handler type specialisation for zero arg.
	template<typename Allocator, typename ReturnType>
	struct handler_type<rf_task_t<Allocator>, ReturnType()> {
		typedef detail::promise_handler<void> type;
	};

	// Handler type specialisation for one arg.
	template<typename Allocator, typename ReturnType, typename Arg1>
	struct handler_type<rf_task_t<Allocator>, ReturnType(Arg1)> {
		typedef detail::promise_handler<Arg1> type;
	};

	// Handler type specialisation for two arg.
	template<typename Allocator, typename ReturnType, typename Arg2>
	struct handler_type<rf_task_t<Allocator>, ReturnType(asio::error_code, Arg2)> {
		typedef detail::promise_handler<Arg2> type;
	};

	template<typename Allocator, typename ReturnType>
	struct handler_type<rf_task_t<Allocator>, ReturnType(asio::error_code)> {
		typedef detail::promise_handler<void> type;
	};

}  // namespace asio

#include "asio/detail/pop_options.hpp"
