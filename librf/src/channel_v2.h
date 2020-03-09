#pragma once

RESUMEF_NS
{
namespace detail
{
	template<class _Ty, class _Opty>
	struct channel_impl_v2;
}	//namespace detail

inline namespace channel_v2
{
	//如果channel缓存的元素不能凭空产生，或者产生代价较大，则推荐第二个模板参数使用true。从而减小不必要的开销。
	template<class _Ty = bool, bool _Option = false>
	struct channel_t
	{
		struct [[nodiscard]] read_awaiter;
		struct [[nodiscard]] write_awaiter;

		channel_t(size_t max_counter = 1);

		size_t capacity() const noexcept;

		read_awaiter operator co_await() const noexcept;
		read_awaiter read() const noexcept;

		template<class U>
		write_awaiter write(U&& val) const noexcept(std::is_move_constructible_v<U>);
		template<class U>
		write_awaiter operator << (U&& val) const noexcept(std::is_move_constructible_v<U>);

		using value_type = _Ty;

		static constexpr bool use_option = _Option;
		using optional_type = std::conditional_t<use_option, std::optional<value_type>, value_type>;
		using channel_type = detail::channel_impl_v2<value_type, optional_type>;
		using lock_type = typename channel_type::lock_type;

		channel_t(const channel_t&) = default;
		channel_t(channel_t&&) = default;
		channel_t& operator = (const channel_t&) = default;
		channel_t& operator = (channel_t&&) = default;
	private:
		std::shared_ptr<channel_type> _chan;
	};

	//不支持channel_t<void>
	template<bool _Option>
	struct channel_t<void, _Option>
	{
	};

	using semaphore_t = channel_t<bool, false>;

}	//namespace channel_v2
}	//RESUMEF_NS
