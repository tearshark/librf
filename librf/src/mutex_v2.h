#pragma once

RESUMEF_NS
{
	namespace detail
	{
		struct mutex_v2_impl;
	}

	inline namespace mutex_v2
	{
		struct [[nodiscard]] scoped_lock_mutex_t;
		struct [[nodiscard]] scoped_unlock_range_t;

		//支持递归的锁
		struct mutex_t
		{
			typedef std::shared_ptr<detail::mutex_v2_impl> mutex_impl_ptr;
			typedef std::chrono::system_clock clock_type;

			mutex_t();
			mutex_t(std::adopt_lock_t) noexcept;
			~mutex_t() noexcept;

			struct [[nodiscard]] awaiter;
			awaiter/*scoped_lock_mutex_t*/ lock() const noexcept;
			awaiter/*scoped_lock_mutex_t*/ operator co_await() const noexcept;

			struct [[nodiscard]] try_awaiter;
			//co_await try_lock()获得是否加锁成功。此操作无论成功与否都会立即返回。
			//如果加锁成功，则需要调用co_await unlock()解锁。或者使用unlock(root_state())解锁。
			//如果加锁失败，且要循环尝试加锁，则最好调用co_await yield()让出一次调度。否则，可能造成本调度器死循环。
			try_awaiter/*bool*/ try_lock() const noexcept;

			//此操作立即返回
			struct [[nodiscard]] unlock_awaiter;
			unlock_awaiter/*void*/ unlock() const noexcept;


			struct [[nodiscard]] timeout_awaiter;
			template <class _Rep, class _Period>
			timeout_awaiter/*bool*/ try_lock_for(const std::chrono::duration<_Rep, _Period>& dt) const noexcept;
			template <class _Rep, class _Period>
			timeout_awaiter/*bool*/ try_lock_until(const std::chrono::time_point<_Rep, _Period>& tp) const noexcept;


			void lock(void* unique_address) const;
			bool try_lock(void* unique_address) const;
			template <class _Rep, class _Period>
			bool try_lock_for(const std::chrono::duration<_Rep, _Period>& dt, void* unique_address);
			template <class _Rep, class _Period>
			bool try_lock_until(const std::chrono::time_point<_Rep, _Period>& tp, void* unique_address);
			void unlock(void* unique_address) const;

			struct _MutexAwaitAssembleT;

			template<class... _Mtxs
				, typename = std::enable_if_t<std::conjunction_v<std::is_same<std::remove_cvref_t<_Mtxs>, mutex_t>...>>
			>
			static future_t<scoped_unlock_range_t> lock(_Mtxs&... mtxs);

			template<class... _Mtxs
				, typename = std::enable_if_t<std::conjunction_v<std::is_same<std::remove_cvref_t<_Mtxs>, mutex_t>...>>
			>
			static scoped_unlock_range_t lock(void* unique_address, _Mtxs&... mtxs);

			template<class... _Mtxs
				, typename = std::enable_if_t<std::conjunction_v<std::is_same<std::remove_cvref_t<_Mtxs>, mutex_t>...>>
			>
			static void unlock(void* unique_address, _Mtxs&... mtxs)
			{
				unlock_address(unique_address, mtxs...);
			}

			mutex_t(const mutex_t&) = default;
			mutex_t(mutex_t&&) = default;
			mutex_t& operator = (const mutex_t&) = default;
			mutex_t& operator = (mutex_t&&) = default;
		private:
			friend struct scoped_lock_mutex_t;
			mutex_impl_ptr _mutex;

			template<class... _Mtxs
				, typename = std::enable_if_t<std::conjunction_v<std::is_same<std::remove_cvref_t<_Mtxs>, mutex_t>...>>
			>
			static void unlock_address(void* unique_address, mutex_t& _First, _Mtxs&... _Rest);
			static void unlock_address(void*) {}
		};
	}
}
