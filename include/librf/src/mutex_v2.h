#pragma once

namespace librf
{
#ifndef DOXYGEN_SKIP_PROPERTY
	namespace detail
	{
		struct mutex_v2_impl;
	}
#endif	//DOXYGEN_SKIP_PROPERTY

	/**
		* @brief 提示手工解锁，故相关的lock()函数不再返回batch_unlock_t。
		*/
	struct adopt_manual_unlock_t{};

	/**
		* @brief 提示手工解锁，故相关的lock()函数不再返回batch_unlock_t。
		*/
	constexpr adopt_manual_unlock_t adopt_manual_unlock;

	/**
		* @brief 在析构的时候自动解锁mutex_t的辅助类。
		*/
	template<class... _Mtxs>
	struct batch_unlock_t;

	/**
		* @brief 支持递归的锁。
		* @details 锁被本协程所在的跟协程所拥有。支持在跟协程下的所有协程里递归加锁。
		*/
	struct mutex_t
	{
		bool is_locked() const;

		struct lock_awaiter;
		struct [[nodiscard]] awaiter;
		struct [[nodiscard]] manual_awaiter;

		/**
			* @brief 在协程中加锁，如果不能立即获得锁，则阻塞当前协程。但不会阻塞当前线程。
			* @return [co_await] batch_unlock_t
			*/
		awaiter/*batch_unlock_t*/ lock() const noexcept;
			
		/**
			* @brief 在协程中加锁。
			* @see 等同调用 co_await lock()。
			* @return [co_await] batch_unlock_t
			*/
		awaiter/*batch_unlock_t*/ operator co_await() const noexcept;

		/**
			* @brief 在协程中加锁，如果不能立即获得锁，则阻塞当前协程。但不会阻塞当前线程。
			* @details 需要随后调用unlock()函数解锁。lock()/unlock()调用必须在同一个跟协程下配对调用。
			* @param manual_unlock_tag 提示手工解锁
			* @return [co_await] void
			*/
		manual_awaiter/*void*/ lock(adopt_manual_unlock_t manual_unlock_tag) const noexcept;


		struct [[nodiscard]] try_awaiter;

		/**
			* @brief 尝试在协程中加锁。此操作无论成功与否都会立即返回，不会有协程切换。
			* @details 如果加锁成功，则需要调用co_await unlock()解锁。或者使用unlock(librf_root_state())解锁。\n
			* 如果加锁失败，且要循环尝试加锁，则最好调用co_await yield()让出一次调度。否则，可能造成本调度器死循环。
			* @return [co_await] bool
			*/
		try_awaiter/*bool*/ try_lock() const noexcept;

		struct [[nodiscard]] unlock_awaiter;

		/**
			* @brief 在协程中解锁。此操作立即返回，不会有协程切换。
			* @return [co_await] void
			*/
		unlock_awaiter/*void*/ unlock() const noexcept;


		struct [[nodiscard]] timeout_awaiter;

		/**
			* @brief 在协程中尝试加锁，直到超时。如果不能立即获得锁，则阻塞当前协程。但不会阻塞当前线程。
			* @param dt 超时时长
			* @return [co_await] bool
			*/
		template <class _Rep, class _Period>
		timeout_awaiter/*bool*/ try_lock_for(const std::chrono::duration<_Rep, _Period>& dt) const noexcept;

		/**
			* @brief 在协程中尝试加锁，直到超时。如果不能立即获得锁，则阻塞当前协程。但不会阻塞当前线程。
			* @param tp 超时时刻
			* @return [co_await] bool
			*/
		template <class _Rep, class _Period>
		timeout_awaiter/*bool*/ try_lock_until(const std::chrono::time_point<_Rep, _Period>& tp) const noexcept;


		/**
			* @brief 在非协程中加锁。如果不能立即获得锁，则反复尝试，直到获得锁。故会阻塞当前协程
			* @param unique_address 代表获得锁的拥有者。此地址应当与随后的unlock()的地址一致。\n
			* 一般做法，是申明一个跟当前线程关联的局部变量，以此局部变量的地址为参数。
			*/
		void lock(void* unique_address) const;

		/**
			* @brief 尝试在非协程中加锁。此操作无论成功与否都会立即返回。
			* @param unique_address 代表获得锁的拥有者。
			*/
		bool try_lock(void* unique_address) const;

		/**
			* @brief 尝试在非协程中加锁，直到超时。如果不能立即获得锁，则反复尝试，直到获得锁或超时。故会阻塞当前协程
			* @param dt 超时时长
			* @param unique_address 代表获得锁的拥有者。
			*/
		template <class _Rep, class _Period>
		bool try_lock_for(const std::chrono::duration<_Rep, _Period>& dt, void* unique_address);

		/**
			* @brief 尝试在非协程中加锁，直到超时。如果不能立即获得锁，则反复尝试，直到获得锁或超时。故会阻塞当前协程
			* @param tp 超时时刻
			* @param unique_address 代表获得锁的拥有者。
			*/
		template <class _Rep, class _Period>
		bool try_lock_until(const std::chrono::time_point<_Rep, _Period>& tp, void* unique_address);

		/**
			* @brief 在非协程中解锁。立即返回。由于立即返回，也可在协程中如此使用：mtx.unlock(librf_root_state())
			* @param unique_address 代表获得锁的拥有者。
			*/
		void unlock(void* unique_address) const;


		/**
			* @brief 在协程中，无死锁的批量加锁。不会阻塞当前线程。直到获得所有锁之前，会阻塞当前协程。
			* @param mtxs... 需要获得的锁列表。
			* @return [co_await] batch_unlock_t
			*/
		template<class... _Mtxs> requires(std::same_as<_Mtxs, mutex_t> && ...)
		static future_t<batch_unlock_t<_Mtxs...>> lock(_Mtxs&... mtxs);

		/**
			* @brief 在协程中，无死锁的批量加锁。不会阻塞当前线程。直到获得所有锁之前，会阻塞当前协程。
			* @param manual_unlock_tag 提示手工解锁
			* @param mtxs... 需要获得的锁列表。
			* @return [co_await] void
			*/
		template<class... _Mtxs> requires(std::same_as<_Mtxs, mutex_t> && ...)
		static future_t<> lock(adopt_manual_unlock_t manual_unlock_tag, _Mtxs&... mtxs);

		/**
			* @brief 在协程中批量解锁。如果可能，使用unlock(librf_root_state(), mtxs...)来替代。
			* @param mtxs... 需要解锁的锁列表。
			* @return [co_await] void
			*/
		template<class... _Mtxs> requires(std::same_as<_Mtxs, mutex_t> && ...)
		static future_t<> unlock(_Mtxs&... mtxs);


		/**
			* @brief 在非协程中，无死锁的批量加锁。会阻塞当前线程，直到获得所有锁为止。
			* @param unique_address 代表获得锁的拥有者。
			* @param mtxs... 需要获得的锁列表。
			* @return batch_unlock_t
			*/
		template<class... _Mtxs> requires(std::same_as<_Mtxs, mutex_t> && ...)
		static batch_unlock_t<_Mtxs...> lock(void* unique_address, _Mtxs&... mtxs);

		/**
			* @brief 在非协程中，无死锁的批量加锁。会阻塞当前线程，直到获得所有锁为止。
			* @param manual_unlock_tag 提示手工解锁
			* @param unique_address 代表获得锁的拥有者。
			* @param mtxs... 需要获得的锁列表。
			*/
		template<class... _Mtxs> requires(std::same_as<_Mtxs, mutex_t> && ...)
		static void lock(adopt_manual_unlock_t manual_unlock_tag, void* unique_address, _Mtxs&... mtxs);

		/**
			* @brief 在非协程中批量解锁。立即返回。由于立即返回，也可在协程中如此使用：unlock(librf_root_state(), mtxs...)
			* @param unique_address 代表获得锁的拥有者。
			* @param mtxs... 需要解锁的锁列表。
			*/
		template<class... _Mtxs> requires(std::same_as<_Mtxs, mutex_t> && ...)
		static void unlock(void* unique_address, _Mtxs&... mtxs);

		LIBRF_API mutex_t();

		/**
			* @brief 构造一个无效的mutex_t。
			*/
		LIBRF_API mutex_t(std::adopt_lock_t) noexcept;
		LIBRF_API ~mutex_t() noexcept;

		mutex_t(const mutex_t&) = default;
		mutex_t(mutex_t&&) = default;
		mutex_t& operator = (const mutex_t&) = default;
		mutex_t& operator = (mutex_t&&) = default;

#ifndef DOXYGEN_SKIP_PROPERTY
		typedef std::shared_ptr<detail::mutex_v2_impl> mutex_impl_ptr;
		typedef std::chrono::system_clock clock_type;
	private:
		struct _MutexAwaitAssembleT;

		template<class... _Mtxs> friend struct batch_unlock_t;

		mutex_impl_ptr _mutex;
#endif	//DOXYGEN_SKIP_PROPERTY
	};
}
