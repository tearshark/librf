#pragma once

#ifndef DOXYGEN_SKIP_PROPERTY
RESUMEF_NS
{
namespace detail
{
	template<class _Ty, class _Opty>
	struct channel_impl_v2;
}	//namespace detail

inline namespace channel_v2
{
#endif	//DOXYGEN_SKIP_PROPERTY

	/**
	 * @brief 可传递数据的模板信号量。
	 * @remarks 不支持数据类型为void的特例化。
	 * @param _Ty 传递的数据类型。要求此类型至少支持移动构造和移动赋值。
	 * @param _Optional 内部是否采用std::optional<>来存数据。\n
	 * 默认不是POD类型则采用std::optional<>。如果channel缓存的元素不能凭空产生，或者产生代价较大，则推荐将此参数设置为true，从而减小不必要的开销。
	 * @param _OptimizationThread 针对多线程优化。目前此算法提升效率不稳定，需要自行根据实际情况决定。
	 */
	template<class _Ty = bool, bool _Optional = !std::is_pod_v<_Ty>, bool _OptimizationThread = false>
	struct channel_t
	{
		static_assert((std::is_copy_constructible_v<_Ty>&& std::is_copy_assignable_v<_Ty>) ||
			(std::is_move_constructible_v<_Ty> && std::is_move_assignable_v<_Ty>));

		struct [[nodiscard]] read_awaiter;
		struct [[nodiscard]] write_awaiter;

		/**
		 * @brief 构造函数。
		 * @param cache_size 缓存的数量。0 表示内部无缓存。
		 */
		channel_t(size_t cache_size = 1);

		/**
		 * @brief 获得缓存数量。
		 */
		size_t capacity() const noexcept;

		/**
		 * @brief 在协程中从channel_t里读取一个数据。参考read()函数
		 */
		read_awaiter operator co_await() const noexcept;

		/**
		 * @brief 在协程中从channel_t里读取一个数据
		 * @details 如果没有写入数据，则会阻塞协程。
		 * @remarks 无缓冲的时候，先读后写，不再抛channel_exception异常。这是跟channel_v1的区别。\n
		 * 在非协程中也可以使用。如果不能立即读取成功，则会阻塞线程。\n
		 * 但如此用法并不能获得读取的结果，仅仅用作同步手段。
		 * @return [co_await] value_type
		 */
		read_awaiter read() const noexcept;

		/**
		 * @brief 在协程中向channel_t里写入一个数据。参考write()函数
		 */
		template<class U
#ifndef DOXYGEN_SKIP_PROPERTY
			COMMA_RESUMEF_ENABLE_IF(std::is_constructible_v<value_type, U&&>)
#endif	//DOXYGEN_SKIP_PROPERTY
		>
#ifndef DOXYGEN_SKIP_PROPERTY
		RESUMEF_REQUIRES(std::is_constructible_v<_Ty, U&&>)
#endif	//DOXYGEN_SKIP_PROPERTY
		write_awaiter operator << (U&& val) const noexcept(std::is_move_constructible_v<U>);

		/**
		 * @brief 在协程中向channel_t里写入一个数据。
		 * @details 在没有读操作等待时，且数据缓冲区满的情况下，则会阻塞协程。
		 * @remarks 在非协程中也可以使用。如果不能立即写入成功，则会阻塞线程。
		 * @param val 写入的数据。必须是可以成功构造_Ty(val)的类型。
		 * @return [co_await] void
		 */
		template<class U
#ifndef DOXYGEN_SKIP_PROPERTY
			COMMA_RESUMEF_ENABLE_IF(std::is_constructible_v<value_type, U&&>)
#endif	//DOXYGEN_SKIP_PROPERTY
		>
#ifndef DOXYGEN_SKIP_PROPERTY
		RESUMEF_REQUIRES(std::is_constructible_v<_Ty, U&&>)
#endif	//DOXYGEN_SKIP_PROPERTY
		write_awaiter write(U&& val) const noexcept(std::is_move_constructible_v<U>);


#ifndef DOXYGEN_SKIP_PROPERTY
		using value_type = _Ty;

		static constexpr bool use_optional = _Optional;
		static constexpr bool optimization_for_multithreading = _OptimizationThread;

		using optional_type = std::conditional_t<use_optional, std::optional<value_type>, value_type>;
		using channel_type = detail::channel_impl_v2<value_type, optional_type>;
		using lock_type = typename channel_type::lock_type;

		channel_t(const channel_t&) = default;
		channel_t(channel_t&&) = default;
		channel_t& operator = (const channel_t&) = default;
		channel_t& operator = (channel_t&&) = default;
	private:
		std::shared_ptr<channel_type> _chan;
#endif	//DOXYGEN_SKIP_PROPERTY
	};


#ifndef DOXYGEN_SKIP_PROPERTY
	//不支持channel_t<void>
	template<bool _Option, bool _OptimizationThread>
	struct channel_t<void, _Option, _OptimizationThread>
	{
	};
#endif	//DOXYGEN_SKIP_PROPERTY

	/**
	 * @brief 利用channel_t重定义的信号量。
	 */
	using semaphore_t = channel_t<bool, false, true>;

}	//namespace channel_v2
}	//RESUMEF_NS
