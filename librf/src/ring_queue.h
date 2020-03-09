#pragma once

#include <memory>
#include <atomic>
#include <cassert>
#include <optional>
#include "src/spinlock.h"

//使用自旋锁完成的线程安全的环形队列。
//支持多个线程同时push和pop。
//_Option : 如果队列保存的数据不支持拷贝只支持移动，则需要设置为true；或者数据希望pop后销毁，都需要设置为true。
//_Sty : 内存保持数量和索引的整数类型。用于外部控制队列的结构体大小。
template<class _Ty, bool _Option = false, class _Sty = uint32_t>
struct ring_queue
{
	using value_type = _Ty;
	using size_type = _Sty;

	static constexpr bool use_option = _Option;
	using optional_type = std::conditional_t<use_option, std::optional<value_type>, value_type>;
public:
	ring_queue(size_t sz);

	ring_queue(const ring_queue&) = delete;
	ring_queue(ring_queue&&) = default;
	ring_queue& operator =(const ring_queue&) = delete;
	ring_queue& operator =(ring_queue&&) = default;

	auto size() const noexcept->size_type;
	auto capacity() const noexcept->size_type;
	bool empty() const noexcept;
	bool full() const noexcept;
	template<class U>
	bool try_push(U&& value) noexcept(std::is_nothrow_move_assignable_v<U>);
	bool try_pop(value_type& value) noexcept(std::is_nothrow_move_assignable_v<value_type>);
private:
	using container_type = std::conditional_t<std::is_same_v<value_type, bool>, std::unique_ptr<optional_type[]>, std::vector<optional_type>>;
	container_type m_bufferPtr;
	size_type m_bufferSize;

	size_type m_writeIndex;
	size_type m_readIndex;
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
	std::atomic<size_type> m_count;
#endif

	auto nextIndex(size_type a_count) const noexcept->size_type;
};

template<class _Ty, bool _Option, class _Sty>
ring_queue<_Ty, _Option, _Sty>::ring_queue(size_t sz)
	: m_bufferSize(static_cast<size_type>(sz + 1))
	, m_writeIndex(0)
	, m_readIndex(0)
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
	, m_count(0)
#endif
{
	if constexpr (std::is_same_v<value_type, bool>)
		m_bufferPtr = container_type{ new optional_type[sz + 1] };
	else
		m_bufferPtr.resize(sz + 1);

	assert(sz < (std::numeric_limits<size_type>::max)());
}

template<class _Ty, bool _Option, class _Sty>
auto ring_queue<_Ty, _Option, _Sty>::nextIndex(size_type a_count) const noexcept->size_type
{
	return static_cast<size_type>((a_count + 1) % m_bufferSize);
}

template<class _Ty, bool _Option, class _Sty>
auto ring_queue<_Ty, _Option, _Sty>::size() const noexcept->size_type
{
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
	return m_count.load(std::memory_order_acquire);
#else
	if (m_writeIndex >= m_readIndex)
		return (m_writeIndex - m_readIndex);
	else
		return (m_bufferSize + m_writeIndex - m_readIndex);
#endif // _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
}

template<class _Ty, bool _Option, class _Sty>
auto ring_queue<_Ty, _Option, _Sty>::capacity() const noexcept->size_type
{
	return m_bufferSize - 1;
}

template<class _Ty, bool _Option, class _Sty>
bool ring_queue<_Ty, _Option, _Sty>::empty() const noexcept
{
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
	return m_count.load(std::memory_order_acquire) == 0;
#else
	return m_writeIndex == m_readIndex;
#endif // _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
}

template<class _Ty, bool _Option, class _Sty>
bool ring_queue<_Ty, _Option, _Sty>::full() const noexcept
{
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
	return (m_count.load(std::memory_order_acquire) == (m_bufferSize - 1));
#else
	return nextIndex(m_writeIndex) == m_readIndex;
#endif // _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
}

template<class _Ty, bool _Option, class _Sty>
template<class U>
bool ring_queue<_Ty, _Option, _Sty>::try_push(U&& value) noexcept(std::is_nothrow_move_assignable_v<U>)
{
	auto nextWriteIndex = nextIndex(m_writeIndex);
	if (nextWriteIndex == m_readIndex)
		return false;

	assert(m_writeIndex < m_bufferSize);

	m_bufferPtr[m_writeIndex] = std::move(value);
	m_writeIndex = nextWriteIndex;

#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
	m_count.fetch_add(1, std::memory_order_acq_rel);
#endif
	return true;
}

template<class _Ty, bool _Option, class _Sty>
bool ring_queue<_Ty, _Option, _Sty>::try_pop(value_type& value) noexcept(std::is_nothrow_move_assignable_v<value_type>)
{
	if (m_readIndex == m_writeIndex)
		return false;

	optional_type& ov = m_bufferPtr[m_readIndex];
	if constexpr (use_option)
	{
		value = std::move(ov.value());
		ov = std::nullopt;
	}
	else
	{
		value = std::move(ov);
	}

	m_readIndex = nextIndex(m_readIndex);

#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
	m_count.fetch_sub(1, std::memory_order_acq_rel);
#endif
	return true;
}
