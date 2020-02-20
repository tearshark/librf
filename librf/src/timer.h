
#pragma once

#include "def.h"

RESUMEF_NS
{
	struct timer_manager;
	typedef std::shared_ptr<timer_manager> timer_mgr_ptr;
	typedef std::weak_ptr<timer_manager> timer_mgr_wptr;

	namespace detail
	{
		typedef std::chrono::system_clock timer_clock_type;
		typedef std::function<void(bool)> timer_callback_type;

		struct timer_target : public std::enable_shared_from_this<timer_target>
		{
			friend timer_manager;
		private:
			enum struct State : uint32_t
			{
				Invalid,
				Added,
				Runing,
			};
			timer_clock_type::time_point	tp;
			timer_callback_type				cb;
			State							st = State::Invalid;
#if _DEBUG
		private:
			timer_manager *					_manager = nullptr;
#endif
		public:
			timer_target(const timer_clock_type::time_point & tp_, const timer_callback_type & cb_)
				: tp(tp_)
				, cb(cb_)
			{
			}
			timer_target(const timer_clock_type::time_point & tp_, timer_callback_type && cb_)
				: tp(tp_)
				, cb(std::forward<timer_callback_type>(cb_))
			{
			}
		private:
			timer_target() = delete;
			timer_target(const timer_target &) = delete;
			timer_target(timer_target && right_) = delete;
			timer_target & operator = (const timer_target &) = delete;
			timer_target & operator = (timer_target && right_) = delete;
		};
		typedef std::shared_ptr<timer_target> timer_target_ptr;
		typedef std::weak_ptr<timer_target> timer_target_wptr;
	}

	struct timer_handler
	{
	private:
		timer_mgr_wptr				_manager;
		detail::timer_target_wptr	_target;
	public:
		timer_handler() = default;
		timer_handler(const timer_handler &) = default;
		timer_handler(timer_handler && right_) noexcept;
		timer_handler & operator = (const timer_handler &) = default;
		timer_handler & operator = (timer_handler && right_) noexcept;

		timer_handler(timer_manager * manager_, const detail::timer_target_ptr & target_);

		void reset();
		bool stop();
		bool expired() const;
	};

	struct timer_manager : public std::enable_shared_from_this<timer_manager>
	{
		typedef detail::timer_target timer_target;
		typedef detail::timer_target_ptr timer_target_ptr;
		typedef detail::timer_clock_type clock_type;
		typedef clock_type::duration duration_type;
		typedef clock_type::time_point time_point_type;

		typedef std::vector<timer_target_ptr> timer_vector_type;
		typedef std::multimap<clock_type::time_point, timer_target_ptr> timer_map_type;
	protected:
		timer_vector_type	_added_timers;
	public:
		timer_map_type		_runing_timers;

		timer_manager();
		~timer_manager();

		template<class _Rep, class _Period, class _Cb>
		timer_target_ptr add(const std::chrono::duration<_Rep, _Period> & dt_, _Cb && cb_)
		{
			return add_(std::chrono::duration_cast<duration_type>(dt_), std::forward<_Cb>(cb_));
		}
		template<class _Clock, class _Duration = typename _Clock::duration, class _Cb>
		timer_target_ptr add(const std::chrono::time_point<_Clock, _Duration> & tp_, _Cb && cb_)
		{
			return add_(std::chrono::time_point_cast<duration_type>(tp_), std::forward<_Cb>(cb_));
		}
		template<class _Rep, class _Period, class _Cb>
		timer_handler add_handler(const std::chrono::duration<_Rep, _Period> & dt_, _Cb && cb_)
		{
			return{ this, add(dt_, std::forward<_Cb>(cb_)) };
		}
		template<class _Clock, class _Duration = typename _Clock::duration, class _Cb>
		timer_handler add_handler(const std::chrono::time_point<_Clock, _Duration> & tp_, _Cb && cb_)
		{
			return{ this, add(tp_, std::forward<_Cb>(cb_)) };
		}

		bool stop(const timer_target_ptr & sptr);

		inline bool empty() const
		{
			return _added_timers.empty() && _runing_timers.empty();
		}
		void clear();
		void update();

		template<class _Cb>
		timer_target_ptr add_(const duration_type & dt_, _Cb && cb_)
		{
			return add_(std::make_shared<timer_target>(clock_type::now() + dt_, std::forward<_Cb>(cb_)));
		}
		template<class _Cb>
		timer_target_ptr add_(const time_point_type & tp_, _Cb && cb_)
		{
			return add_(std::make_shared<timer_target>(tp_, std::forward<_Cb>(cb_)));
		}
	private:
		timer_target_ptr add_(const timer_target_ptr & sptr);
		static void call_target_(const timer_target_ptr & sptr, bool canceld);
	};


	inline timer_handler::timer_handler(timer_manager * manager_, const detail::timer_target_ptr & target_)
		: _manager(manager_->shared_from_this())
		, _target(target_)
	{
	}
	inline timer_handler::timer_handler(timer_handler && right_) noexcept
		: _manager(std::move(right_._manager))
		, _target(std::move(right_._target))
	{
	}

	inline timer_handler & timer_handler::operator = (timer_handler && right_) noexcept
	{
		if (this != &right_)
		{
			_manager = std::move(right_._manager);
			_target = std::move(right_._target);
		}
		return *this;
	}

	inline void timer_handler::reset()
	{
		_manager.reset();
		_target.reset();
	}

	inline bool timer_handler::stop()
	{
		bool result = false;
		
		if (!_target.expired())
		{
			auto sptr = _manager.lock();
			if (sptr)
				result = sptr->stop(_target.lock());
			_target.reset();
		}
		_manager.reset();

		return result;
	}

	inline bool timer_handler::expired() const
	{
		return _target.expired();
	}
}