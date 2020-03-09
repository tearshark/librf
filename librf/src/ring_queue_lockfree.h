#pragma once

RESUMEF_NS
{
	//目前无法解决三个索引数值回绕导致的问题
	//如果为了避免索引回绕的问题，索引采用uint64_t类型，
	//则在与spinlock<T, false, uint32_t>版本的对比中速度反而慢了
	//pop时无法使用move语义来获取数据。因为算法要求先获取值，且获取后有可能失败，从而重新获取其它值。
	template<class _Ty, class _Sty = uint32_t>
	struct ring_queue_lockfree
	{
		using value_type = _Ty;
		using size_type = _Sty;
	public:
		ring_queue_lockfree(size_t sz);

		ring_queue_lockfree(const ring_queue_lockfree&) = delete;
		ring_queue_lockfree(ring_queue_lockfree&&) = default;
		ring_queue_lockfree& operator =(const ring_queue_lockfree&) = delete;
		ring_queue_lockfree& operator =(ring_queue_lockfree&&) = default;

		auto size() const noexcept->size_type;
		auto capacity() const noexcept->size_type;
		bool empty() const noexcept;
		bool full() const noexcept;
		template<class U>
		bool try_push(U&& value) noexcept(std::is_nothrow_move_constructible_v<U>);
		bool try_pop(value_type& value) noexcept(std::is_nothrow_move_constructible_v<value_type>);
	private:
		std::unique_ptr<value_type[]> m_bufferPtr;
		size_type m_bufferSize;

		std::atomic<size_type> m_writeIndex;		//Where a new element will be inserted to.
		std::atomic<size_type> m_readIndex;			//Where the next element where be extracted from.
		std::atomic<size_type> m_maximumReadIndex;	//It points to the place where the latest "commited" data has been inserted. 
													//If it's not the same as writeIndex it means there are writes pending to be "commited" to the queue, 
													//that means that the place for the data was reserved (the index in the array) 
													//but the data is still not in the queue, 
													//so the thread trying to read will have to wait for those other threads to 
													//save the data into the queue.
	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		std::atomic<size_type> m_count;
	#endif

		auto countToIndex(size_type a_count) const noexcept->size_type;
		auto nextIndex(size_type a_count) const noexcept->size_type;
	};

	template<class _Ty, class _Sty>
	ring_queue_lockfree<_Ty, _Sty>::ring_queue_lockfree(size_t sz)
		: m_bufferPtr(new value_type[sz + 1])
		, m_bufferSize(static_cast<size_type>(sz + 1))
		, m_writeIndex(0)
		, m_readIndex(0)
		, m_maximumReadIndex(0)
	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		, m_count(0)
	#endif
	{
		assert(sz < (std::numeric_limits<size_type>::max)());
	}

	template<class _Ty, class _Sty>
	auto ring_queue_lockfree<_Ty, _Sty>::countToIndex(size_type a_count) const noexcept->size_type
	{
		//return (a_count % m_bufferSize);
		return a_count;
	}

	template<class _Ty, class _Sty>
	auto ring_queue_lockfree<_Ty, _Sty>::nextIndex(size_type a_count) const noexcept->size_type
	{
		//return static_cast<size_type>((a_count + 1));
		return static_cast<size_type>((a_count + 1) % m_bufferSize);
	}

	template<class _Ty, class _Sty>
	auto ring_queue_lockfree<_Ty, _Sty>::size() const noexcept->size_type
	{
	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		return m_count.load();
	#else
		auto currentWriteIndex = m_maximumReadIndex.load(std::memory_order_acquire);
		currentWriteIndex = countToIndex(currentWriteIndex);

		auto currentReadIndex = m_readIndex.load(std::memory_order_acquire);
		currentReadIndex = countToIndex(currentReadIndex);

		if (currentWriteIndex >= currentReadIndex)
			return (currentWriteIndex - currentReadIndex);
		else
			return (m_bufferSize + currentWriteIndex - currentReadIndex);
	#endif // _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
	}

	template<class _Ty, class _Sty>
	auto ring_queue_lockfree<_Ty, _Sty>::capacity() const noexcept->size_type
	{
		return m_bufferSize - 1;
	}

	template<class _Ty, class _Sty>
	bool ring_queue_lockfree<_Ty, _Sty>::empty() const noexcept
	{
	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		return m_count.load() == 0;
	#else
		auto currentWriteIndex = m_maximumReadIndex.load(std::memory_order_acquire);
		auto currentReadIndex = m_readIndex.load(std::memory_order_acquire);
		return countToIndex(currentWriteIndex) == countToIndex(currentReadIndex);
	#endif // _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
	}

	template<class _Ty, class _Sty>
	bool ring_queue_lockfree<_Ty, _Sty>::full() const noexcept
	{
	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		return (m_count.load() == (m_bufferSize - 1));
	#else
		auto currentWriteIndex = m_writeIndex.load(std::memory_order_acquire);
		auto currentReadIndex = m_readIndex.load(std::memory_order_acquire);
		return countToIndex(nextIndex(currentWriteIndex)) == countToIndex(currentReadIndex);
	#endif // _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
	}

	template<class _Ty, class _Sty>
	template<class U>
	bool ring_queue_lockfree<_Ty, _Sty>::try_push(U&& value) noexcept(std::is_nothrow_move_constructible_v<U>)
	{
		auto currentWriteIndex = m_writeIndex.load(std::memory_order_acquire);

		do
		{
			if (countToIndex(nextIndex(currentWriteIndex)) == countToIndex(m_readIndex.load(std::memory_order_acquire)))
			{
				// the queue is full
				return false;
			}

			// There is more than one producer. Keep looping till this thread is able 
			// to allocate space for current piece of data
			//
			// using compare_exchange_strong because it isn't allowed to fail spuriously
			// When the compare_exchange operation is in a loop the weak version
			// will yield better performance on some platforms, but here we'd have to
			// load m_writeIndex all over again
		} while (!m_writeIndex.compare_exchange_strong(currentWriteIndex, nextIndex(currentWriteIndex), std::memory_order_acq_rel));

		// Just made sure this index is reserved for this thread.
		m_bufferPtr[countToIndex(currentWriteIndex)] = std::move(value);

		// update the maximum read index after saving the piece of data. It can't
		// fail if there is only one thread inserting in the queue. It might fail 
		// if there is more than 1 producer thread because this operation has to
		// be done in the same order as the previous CAS
		//
		// using compare_exchange_weak because they are allowed to fail spuriously
		// (act as if *this != expected, even if they are equal), but when the
		// compare_exchange operation is in a loop the weak version will yield
		// better performance on some platforms.
		auto savedWriteIndex = currentWriteIndex;
		while (!m_maximumReadIndex.compare_exchange_weak(currentWriteIndex, nextIndex(currentWriteIndex), std::memory_order_acq_rel))
		{
			currentWriteIndex = savedWriteIndex;
			// this is a good place to yield the thread in case there are more
			// software threads than hardware processors and you have more
			// than 1 producer thread
			// have a look at sched_yield (POSIX.1b)
			std::this_thread::yield();
		}

		// The value was successfully inserted into the queue
	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		m_count.fetch_add(1);
	#endif
		return true;
	}

	template<class _Ty, class _Sty>
	bool ring_queue_lockfree<_Ty, _Sty>::try_pop(value_type& value) noexcept(std::is_nothrow_move_constructible_v<value_type>)
	{
		auto currentReadIndex = m_readIndex.load(std::memory_order_acquire);

		for (;;)
		{
			auto idx = countToIndex(currentReadIndex);

			// to ensure thread-safety when there is more than 1 producer 
			// thread a second index is defined (m_maximumReadIndex)
			if (idx == countToIndex(m_maximumReadIndex.load(std::memory_order_acquire)))
			{
				// the queue is empty or
				// a producer thread has allocate space in the queue but is 
				// waiting to commit the data into it
				return false;
			}

			// retrieve the data from the queue
			value = m_bufferPtr[idx];		//但是，这里的方法不适合。如果只支持移动怎么办？

			// try to perfrom now the CAS operation on the read index. If we succeed
			// a_data already contains what m_readIndex pointed to before we 
			// increased it
			if (m_readIndex.compare_exchange_strong(currentReadIndex, nextIndex(currentReadIndex), std::memory_order_acq_rel))
			{
				// got here. The value was retrieved from the queue. Note that the
				// data inside the m_queue array is not deleted nor reseted
	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
				m_count.fetch_sub(1);
	#endif
				return true;
			}

			// it failed retrieving the element off the queue. Someone else must
			// have read the element stored at countToIndex(currentReadIndex)
			// before we could perform the CAS operation        
		}	// keep looping to try again!
	}
}