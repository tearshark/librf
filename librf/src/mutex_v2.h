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
		//锁被本协程所在的跟协程所拥有。支持在跟协程下的所有协程里递归加锁。
		struct mutex_t
		{
			bool is_locked() const;

			struct lock_awaiter;
			struct [[nodiscard]] awaiter;
			struct [[nodiscard]] manual_awaiter;

			/**
			 * @brief 在协程中加锁。
			 * @return [co_await] scoped_unlock_t
			 */
			awaiter/*scoped_unlock_t*/ lock() const noexcept;
			
			/**
			 * @brief 等同调用co_await lock()。
			 * @return [co_await] scoped_unlock_t
			 */
			awaiter/*scoped_unlock_t*/ operator co_await() const noexcept;

			/**
			 * @brief 在协程中加锁。需要随后调用unlock()函数解锁。lock()/unlock()调用必须在同一个跟协程下配对调用。
			 * @return [co_await] void
			 */
			manual_awaiter/*void*/ lock(adopt_manual_unlock_t) const noexcept;

			struct [[nodiscard]] try_awaiter;
			
			/**
			 * @brief 尝试在协程中加锁。此操作无论成功与否都会立即返回。
			 * 如果加锁成功，则需要调用co_await unlock()解锁。或者使用unlock(root_state())解锁。
			 * 如果加锁失败，且要循环尝试加锁，则最好调用co_await yield()让出一次调度。否则，可能造成本调度器死循环。
			 * @return [co_await] bool
			 */
			try_awaiter/*bool*/ try_lock() const noexcept;

			/**
			 * @brief 在协程中解锁。此操作立即返回。
			 * @return [co_await] void
			 */
			struct [[nodiscard]] unlock_awaiter;
			unlock_awaiter/*void*/ unlock() const noexcept;


			struct [[nodiscard]] timeout_awaiter;

			/**
			 * @brief 在协程中尝试加锁，直到超时
			 * @param dt 超时时长
			 * @return [co_await] bool
			 */
			template <class _Rep, class _Period>
			timeout_awaiter/*bool*/ try_lock_for(const std::chrono::duration<_Rep, _Period>& dt) const noexcept;

			/**
			 * @brief 在协程中尝试加锁，直到超时
			 * @param tp 超时时刻
			 * @return [co_await] bool
			 */
			template <class _Rep, class _Period>
			timeout_awaiter/*bool*/ try_lock_until(const std::chrono::time_point<_Rep, _Period>& tp) const noexcept;


			void lock(void* unique_address) const;
			bool try_lock(void* unique_address) const;
			template <class _Rep, class _Period>
			bool try_lock_for(const std::chrono::duration<_Rep, _Period>& dt, void* unique_address);
			template <class _Rep, class _Period>
			bool try_lock_until(const std::chrono::time_point<_Rep, _Period>& tp, void* unique_address);
			void unlock(void* unique_address) const;


			/**
			 * @brief 在协程中，无死锁的批量加锁。捕获阻塞当前线程。直到获得所有锁之前，阻塞当前协程。
			 * @return [co_await] scoped_unlock_t
			 */
			template<class... _Mtxs
				, typename = std::enable_if_t<std::conjunction_v<std::is_same<remove_cvref_t<_Mtxs>, mutex_t>...>>
			>
			static future_t<scoped_unlock_t<_Mtxs...>> lock(_Mtxs&... mtxs);


			/**
			 * @brief 在非协程中，无死锁的批量加锁。会阻塞当前线程，直到获得所有锁为止。
			 * @return scoped_unlock_t
			 */
			template<class... _Mtxs
				, typename = std::enable_if_t<std::conjunction_v<std::is_same<remove_cvref_t<_Mtxs>, mutex_t>...>>
			>
			static scoped_unlock_t<_Mtxs...> lock(void* unique_address, _Mtxs&... mtxs);

			/**
			 * @brief 在非协程中，无死锁的批量加锁。会阻塞当前线程，直到获得所有锁为止。
			 */
			template<class... _Mtxs
				, typename = std::enable_if_t<std::conjunction_v<std::is_same<remove_cvref_t<_Mtxs>, mutex_t>...>>
			>
			static void lock(adopt_manual_unlock_t, void* unique_address, _Mtxs&... mtxs);

			/**
			 * @brief 在非协程中，批量解锁加锁。立即返回。
			 */
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
