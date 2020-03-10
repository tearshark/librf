#pragma once

RESUMEF_NS
{
	template<class _Node, class _Nodeptr = _Node*, class _Sty = uint32_t>
	struct intrusive_link_queue
	{
		using node_type = _Node;
		using node_ptr_type = _Nodeptr;
		using size_type = _Sty;
	public:
		intrusive_link_queue();

		intrusive_link_queue(const intrusive_link_queue&) = delete;
		intrusive_link_queue(intrusive_link_queue&&) = default;
		intrusive_link_queue& operator =(const intrusive_link_queue&) = delete;
		intrusive_link_queue& operator =(intrusive_link_queue&&) = default;

		auto size() const noexcept->size_type;
		bool empty() const noexcept;
		void push_back(node_ptr_type node) noexcept;
		auto try_pop() noexcept->node_ptr_type;
	private:
		node_ptr_type _head;
		node_ptr_type _tail;

	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		std::atomic<size_type> m_count;
	#endif
	};

	template<class _Node, class _Nodeptr, class _Sty>
	intrusive_link_queue<_Node, _Nodeptr, _Sty>::intrusive_link_queue()
		: _head(nullptr)
		, _tail(nullptr)
	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		, m_count(0)
	#endif
	{
	}

	template<class _Node, class _Nodeptr, class _Sty>
	auto intrusive_link_queue<_Node, _Nodeptr, _Sty>::size() const noexcept->size_type
	{
	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		return m_count.load(std::memory_order_acquire);
	#else
		size_type count = 0;
		for (node_type* node = _head; node != nullptr; node = node->next)
			++count;
		return count;
	#endif // _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
	}

	template<class _Node, class _Nodeptr, class _Sty>
	bool intrusive_link_queue<_Node, _Nodeptr, _Sty>::empty() const noexcept
	{
		return _head == nullptr;
	}

	template<class _Node, class _Nodeptr, class _Sty>
	void intrusive_link_queue<_Node, _Nodeptr, _Sty>::push_back(node_ptr_type node) noexcept
	{
		assert(node != nullptr);

		node->_next = nullptr;
		if (_head == nullptr)
		{
			_head = _tail = node;
		}
		else
		{
			_tail->_next = node;
			_tail = node;
		}

	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		m_count.fetch_add(1, std::memory_order_acq_rel);
	#endif
	}

	template<class _Node, class _Nodeptr, class _Sty>
	auto intrusive_link_queue<_Node, _Nodeptr, _Sty>::try_pop() noexcept->node_ptr_type
	{
		if (_head == nullptr)
			return nullptr;

		node_ptr_type node = _head;
		_head = node->_next;
		node->_next = nullptr;

		if (_tail == node)
		{
			assert(node->_next == nullptr);
			_tail = nullptr;
		}

	#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		m_count.fetch_sub(1, std::memory_order_acq_rel);
	#endif

		return node;
	}
}