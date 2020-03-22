#pragma once

RESUMEF_NS
{
	namespace detail
	{
		struct mutex_v2_impl;
	}

	inline namespace mutex_v2
	{
		struct adopt_manual_unlock_t{};
		constexpr adopt_manual_unlock_t adopt_manual_unlock;

		template<class... _Mtxs>
		struct [[nodiscard]] scoped_unlock_t;

		//支持递归的锁
		struct mutex_t
		{
			bool is_locked() const;

			struct lock_awaiter;
			struct [[nodiscard]] awaiter;
			struct [[nodiscard]] manual_awaiter;

			awaiter/*scoped_unlock_t*/ lock() const noexcept;
			awaiter/*scoped_unlock_t*/ operator co_await() const noexcept;
			manual_awaiter/*void*/ lock(adopt_manual_unlock_t) const noexcept;

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


			template<class... _Mtxs
				, typename = std::enable_if_t<std::conjunction_v<std::is_same<remove_cvref_t<_Mtxs>, mutex_t>...>>
			>
			static future_t<scoped_unlock_t<_Mtxs...>> lock(_Mtxs&... mtxs);


			template<class... _Mtxs
				, typename = std::enable_if_t<std::conjunction_v<std::is_same<remove_cvref_t<_Mtxs>, mutex_t>...>>
			>
			static scoped_unlock_t<_Mtxs...> lock(void* unique_address, _Mtxs&... mtxs);

			template<class... _Mtxs
				, typename = std::enable_if_t<std::conjunction_v<std::is_same<remove_cvref_t<_Mtxs>, mutex_t>...>>
			>
			static void unlock(void* unique_address, _Mtxs&... mtxs);

			mutex_t();
			mutex_t(std::adopt_lock_t) noexcept;
			~mutex_t() noexcept;

			mutex_t(const mutex_t&) = default;
			mutex_t(mutex_t&&) = default;
			mutex_t& operator = (const mutex_t&) = default;
			mutex_t& operator = (mutex_t&&) = default;

			typedef std::shared_ptr<detail::mutex_v2_impl> mutex_impl_ptr;
			typedef std::chrono::system_clock clock_type;
		private:
			struct _MutexAwaitAssembleT;

			template<class... _Mtxs> friend struct scoped_unlock_t;

			mutex_impl_ptr _mutex;
		};
	}
}
