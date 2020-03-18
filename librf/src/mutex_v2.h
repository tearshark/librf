#pragma once

RESUMEF_NS
{
	namespace detail
	{
		struct mutex_v2_impl;
	}

	inline namespace mutex_v2
	{
		struct scoped_lock_mutex_t;

		//支持递归的锁
		struct mutex_t
		{
			typedef std::shared_ptr<detail::mutex_v2_impl> mutex_impl_ptr;
			typedef std::chrono::system_clock clock_type;

			mutex_t();
			mutex_t(std::adopt_lock_t) noexcept;
			~mutex_t() noexcept;

			struct [[nodiscard]] awaiter;

			awaiter operator co_await() const noexcept;
			awaiter lock() const noexcept;

			scoped_lock_mutex_t lock(void* unique_address) const;
			bool try_lock(void* unique_address) const;
			void unlock(void* unique_address) const;

			mutex_t(const mutex_t&) = default;
			mutex_t(mutex_t&&) = default;
			mutex_t& operator = (const mutex_t&) = default;
			mutex_t& operator = (mutex_t&&) = default;
		private:
			friend struct scoped_lock_mutex_t;
			mutex_impl_ptr _mutex;
		};
	}
}
