#pragma once

namespace resumef
{
	namespace detail
	{
		struct event_impl;
		typedef _awaker<event_impl> event_awaker;
		typedef std::shared_ptr<event_awaker> event_awaker_ptr;

		struct event_impl : public std::enable_shared_from_this<event_impl>
		{
		private:
			//typedef spinlock lock_type;
			typedef std::recursive_mutex lock_type;

			std::list<event_awaker_ptr> _awakes;
			intptr_t _counter;
			lock_type _lock;
		public:
			event_impl(intptr_t initial_counter_);

			void signal();
			void reset();

			//如果已经触发了awaker,则返回true
			bool wait_(const event_awaker_ptr& awaker);

			template<class callee_t, class dummy_t = std::enable_if<!std::is_same<std::remove_cv_t<callee_t>, event_awaker_ptr>::value>>
			decltype(auto) wait(callee_t&& awaker, dummy_t* dummy_ = nullptr)
			{
				(void)dummy_;
				return wait_(std::make_shared<event_awaker>(std::forward<callee_t>(awaker)));
			}

			event_impl(const event_impl&) = delete;
			event_impl(event_impl&&) = delete;
			event_impl& operator = (const event_impl&) = delete;
			event_impl& operator = (event_impl&&) = delete;
		};
	}

namespace event_v1
{

	//提供一种在协程和非协程之间同步的手段。
	//典型用法是在非协程的线程，或者异步代码里，调用signal()方法触发信号，
	//协程代码里，调用co_await wait()等系列方法等待同步。
	struct event_t
	{
		typedef std::shared_ptr<detail::event_impl> event_impl_ptr;
		typedef std::weak_ptr<detail::event_impl> event_impl_wptr;
		typedef std::chrono::system_clock clock_type;
	private:
		event_impl_ptr _event;
		struct wait_all_ctx;
	public:
		event_t(intptr_t initial_counter_ = 0);

		void signal() const
		{
			_event->signal();
		}
		void reset() const
		{
			_event->reset();
		}



		future_t<bool>
			wait() const;
		template<class _Rep, class _Period>
		future_t<bool>
			wait_for(const std::chrono::duration<_Rep, _Period>& dt) const
		{
			return wait_for_(std::chrono::duration_cast<clock_type::duration>(dt));
		}
		template<class _Clock, class _Duration>
		future_t<bool>
			wait_until(const std::chrono::time_point<_Clock, _Duration>& tp) const
		{
			return wait_until_(std::chrono::time_point_cast<clock_type::duration>(tp));
		}





		template<class _Iter>
		static future_t<intptr_t>
			wait_any(_Iter begin_, _Iter end_)
		{
			return wait_any_(make_event_vector(begin_, end_));
		}
		template<class _Cont>
		static future_t<intptr_t>
			wait_any(const _Cont& cnt_)
		{
			return wait_any_(make_event_vector(std::begin(cnt_), std::end(cnt_)));
		}

		template<class _Rep, class _Period, class _Iter>
		static future_t<intptr_t>
			wait_any_for(const std::chrono::duration<_Rep, _Period>& dt, _Iter begin_, _Iter end_)
		{
			return wait_any_for_(std::chrono::duration_cast<clock_type::duration>(dt), make_event_vector(begin_, end_));
		}
		template<class _Rep, class _Period, class _Cont>
		static future_t<intptr_t>
			wait_any_for(const std::chrono::duration<_Rep, _Period>& dt, const _Cont& cnt_)
		{
			return wait_any_for_(std::chrono::duration_cast<clock_type::duration>(dt), make_event_vector(std::begin(cnt_), std::end(cnt_)));
		}

		template<class _Clock, class _Duration, class _Iter>
		static future_t<intptr_t>
			wait_any_until(const std::chrono::time_point<_Clock, _Duration>& tp, _Iter begin_, _Iter end_)
		{
			return wait_any_until_(std::chrono::time_point_cast<clock_type::duration>(tp), make_event_vector(begin_, end_));
		}
		template<class _Clock, class _Duration, class _Cont>
		static future_t<intptr_t>
			wait_any_until(const std::chrono::time_point<_Clock, _Duration>& tp, const _Cont& cnt_)
		{
			return wait_any_until_(std::chrono::time_point_cast<clock_type::duration>(tp), make_event_vector(std::begin(cnt_), std::end(cnt_)));
		}





		template<class _Iter>
		static future_t<bool>
			wait_all(_Iter begin_, _Iter end_)
		{
			return wait_all_(make_event_vector(begin_, end_));
		}
		template<class _Cont>
		static future_t<bool>
			wait_all(const _Cont& cnt_)
		{
			return wait_all(std::begin(cnt_), std::end(cnt_));
		}

		template<class _Rep, class _Period, class _Iter>
		static future_t<bool>
			wait_all_for(const std::chrono::duration<_Rep, _Period>& dt, _Iter begin_, _Iter end_)
		{
			return wait_all_for_(std::chrono::duration_cast<clock_type::duration>(dt), make_event_vector(begin_, end_));
		}
		template<class _Rep, class _Period, class _Cont>
		static future_t<bool>
			wait_all_for(const std::chrono::duration<_Rep, _Period>& dt, const _Cont& cnt_)
		{
			return wait_all_for_(std::chrono::duration_cast<clock_type::duration>(dt), make_event_vector(std::begin(cnt_), std::end(cnt_)));
		}

		template<class _Clock, class _Duration, class _Iter>
		static future_t<bool>
			wait_all_until(const std::chrono::time_point<_Clock, _Duration>& tp, _Iter begin_, _Iter end_)
		{
			return wait_all_until_(std::chrono::time_point_cast<clock_type::duration>(tp), make_event_vector(begin_, end_));
		}
		template<class _Clock, class _Duration, class _Cont>
		static future_t<bool>
			wait_all_until(const std::chrono::time_point<_Clock, _Duration>& tp, const _Cont& cnt_)
		{
			return wait_all_until_(std::chrono::time_point_cast<clock_type::duration>(tp), make_event_vector(std::begin(cnt_), std::end(cnt_)));
		}



		event_t(const event_t&) = default;
		event_t(event_t&&) = default;
		event_t& operator = (const event_t&) = default;
		event_t& operator = (event_t&&) = default;

	private:
		template<class _Iter>
		static std::vector<event_impl_ptr> make_event_vector(_Iter begin_, _Iter end_)
		{
			std::vector<event_impl_ptr> evts;
			evts.reserve(std::distance(begin_, end_));
			for (auto i = begin_; i != end_; ++i)
				evts.push_back((*i)._event);

			return evts;
		}

		inline future_t<bool> wait_for_(const clock_type::duration& dt) const
		{
			return wait_until_(clock_type::now() + dt);
		}
		future_t<bool> wait_until_(const clock_type::time_point& tp) const;


		static future_t<intptr_t> wait_any_(std::vector<event_impl_ptr>&& evts);
		inline static future_t<intptr_t> wait_any_for_(const clock_type::duration& dt, std::vector<event_impl_ptr>&& evts)
		{
			return wait_any_until_(clock_type::now() + dt, std::forward<std::vector<event_impl_ptr>>(evts));
		}
		static future_t<intptr_t> wait_any_until_(const clock_type::time_point& tp, std::vector<event_impl_ptr>&& evts);


		static future_t<bool> wait_all_(std::vector<event_impl_ptr>&& evts);
		inline static future_t<bool> wait_all_for_(const clock_type::duration& dt, std::vector<event_impl_ptr>&& evts)
		{
			return wait_all_until_(clock_type::now() + dt, std::forward<std::vector<event_impl_ptr>>(evts));
		}
		static future_t<bool> wait_all_until_(const clock_type::time_point& tp, std::vector<event_impl_ptr>&& evts);
	};

}
}
