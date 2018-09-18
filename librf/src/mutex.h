#pragma once

#include "_awaker.h"

namespace resumef
{
	namespace detail
	{
		struct mutex_impl;
		typedef _awaker<mutex_impl> mutex_awaker;
		typedef std::shared_ptr<mutex_awaker> mutex_awaker_ptr;

		struct mutex_impl : public std::enable_shared_from_this<mutex_impl>
		{
		private:
			//typedef spinlock lock_type;
			typedef std::recursive_mutex lock_type;

			std::list<mutex_awaker_ptr> _awakes;
			mutex_awaker_ptr _owner;
			lock_type _lock;
		public:
			RF_API mutex_impl();

			//如果已经触发了awaker,则返回true
			RF_API bool lock_(const mutex_awaker_ptr & awaker);
			RF_API bool try_lock_(const mutex_awaker_ptr & awaker);
			RF_API void unlock();

			template<class callee_t, class dummy_t = std::enable_if<!std::is_same<std::remove_cv_t<callee_t>, mutex_awaker_ptr>::value>>
			decltype(auto) lock(callee_t && awaker, dummy_t * dummy_ = nullptr)
			{
				return lock_(std::make_shared<mutex_awaker>(std::forward<callee_t>(awaker)));
			}

		private:
			mutex_impl(const mutex_impl &) = delete;
			mutex_impl(mutex_impl &&) = delete;
			mutex_impl & operator = (const mutex_impl &) = delete;
			mutex_impl & operator = (mutex_impl &&) = delete;
		};
	}

	struct mutex_t
	{
		typedef std::shared_ptr<detail::mutex_impl> lock_impl_ptr;
		typedef std::weak_ptr<detail::mutex_impl> lock_impl_wptr;
		typedef std::chrono::system_clock clock_type;
	private:
		lock_impl_ptr _locker;
	public:
		RF_API mutex_t();

		void unlock() const
		{
			_locker->unlock();
		}


		RF_API future_t<bool>
			lock() const;
		RF_API bool
			try_lock() const;

/*
		template<class _Rep, class _Period> 
		awaitable_t<bool> 
			try_lock_for(const std::chrono::duration<_Rep, _Period> & dt) const
		{
			return try_lock_for_(std::chrono::duration_cast<clock_type::duration>(dt));
		}
		template<class _Clock, class _Duration> 
		awaitable_t<bool> 
			try_lock_until(const std::chrono::time_point<_Clock, _Duration> & tp) const
		{
			return try_lock_until_(std::chrono::time_point_cast<clock_type::duration>(tp));
		}
*/


		RF_API mutex_t(const mutex_t &) = default;
		RF_API mutex_t(mutex_t &&) = default;
		RF_API mutex_t & operator = (const mutex_t &) = default;
		RF_API mutex_t & operator = (mutex_t &&) = default;
	private:
		inline future_t<bool> try_lock_for_(const clock_type::duration & dt) const
		{
			return try_lock_until_(clock_type::now() + dt);
		}
		RF_API future_t<bool> try_lock_until_(const clock_type::time_point & tp) const;
	};

#if _HAS_CXX17
#define resumf_guard_lock(lker) (lker).lock(); resumef::scoped_lock<resumef::mutex_t> __resumf_guard##lker##__(std::adopt_lock, (lker))
#else
#define resumf_guard_lock(lker) (lker).lock(); resumef::scoped_lock<resumef::mutex_t> __resumf_guard##lker##__((lker), std::adopt_lock)
#endif

}
