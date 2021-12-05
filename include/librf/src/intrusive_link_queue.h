#pragma once

namespace librf
{
	template<class _Node, class _Nextptr = _Node*>
	struct intrusive_link_node
	{
	private:
		_Node* _prev;
		_Nextptr _next;

		template<class _Node2, class _Nodeptr2>
		friend struct intrusive_link_queue;
	};

#define _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE 1

	template<class _Node, class _Nodeptr = _Node*>
	struct intrusive_link_queue
	{
		using node_type = _Node;
		using node_ptr_type = _Nodeptr;
	public:
		intrusive_link_queue();

		intrusive_link_queue(const intrusive_link_queue&) = delete;
		intrusive_link_queue(intrusive_link_queue&&) = default;
		intrusive_link_queue& operator =(const intrusive_link_queue&) = delete;
		intrusive_link_queue& operator =(intrusive_link_queue&&) = default;

		std::size_t size() const noexcept;
		bool empty() const noexcept;
		void push_back(node_ptr_type node) noexcept;
		void push_front(node_ptr_type node) noexcept;
		void erase(node_ptr_type node) noexcept;
		auto try_pop() noexcept->node_ptr_type;
	private:
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		std::atomic<std::size_t> _count;
#endif
		node_ptr_type _header;
		node_ptr_type _tailer;
	};

	template<class _Node, class _Nodeptr>
	intrusive_link_queue<_Node, _Nodeptr>::intrusive_link_queue()
		: _header(nullptr)
		, _tailer(nullptr)
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		, _count(0)
#endif
	{
	}

	template<class _Node, class _Nodeptr>
	std::size_t intrusive_link_queue<_Node, _Nodeptr>::size() const noexcept
	{
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		return _count.load(std::memory_order_acquire);
#else
		std::size_t count = 0;
		for (node_type* node = _header; node != nullptr; node = node->next)
			++count;
		return count;
#endif // _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
	}

	template<class _Node, class _Nodeptr>
	bool intrusive_link_queue<_Node, _Nodeptr>::empty() const noexcept
	{
		return _header == nullptr;
	}

	template<class _Node, class _Nodeptr>
	void intrusive_link_queue<_Node, _Nodeptr>::push_back(node_ptr_type e) noexcept
	{
		assert(e != nullptr);

		e->_prev = _tailer;
		e->_next = nullptr;

		if (!_header)
			_header = e;

		if (_tailer)
			_tailer->_next = e;
		_tailer = e;

#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		_count.fetch_add(1, std::memory_order_acq_rel);
#endif
	}

	template<class _Node, class _Nodeptr>
	void intrusive_link_queue<_Node, _Nodeptr>::push_front(node_ptr_type e) noexcept
	{
		assert(e != nullptr);

		e->_prev = nullptr;
		e->_next = _header;

		if (_header)
			_header->_prev = e;
		_header = e;

		if (!_tailer)
			_tailer = e;

#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		_count.fetch_add(1, std::memory_order_acq_rel);
#endif
	}

	template<class _Node, class _Nodeptr>
	void intrusive_link_queue<_Node, _Nodeptr>::erase(node_ptr_type e) noexcept
	{
		assert(e != nullptr);

		if (_header == e)
			_header = e->_next;
		if (_tailer == e)
			_tailer = e->_prev;

		if (e->_next)
			e->_next->_prev = e->_prev;
		if (e->_prev)
			e->_prev->_next = e->_next;

		e->_prev = nullptr;
		e->_next = nullptr;

#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		_count.fetch_sub(1, std::memory_order_acq_rel);
#endif
	}

	template<class _Node, class _Nodeptr>
	auto intrusive_link_queue<_Node, _Nodeptr>::try_pop() noexcept->node_ptr_type
	{
		if (_header == nullptr)
			return nullptr;

		node_ptr_type node = _header;
		_header = node->_next;
		if (_header != nullptr)
			_header->_prev = nullptr;

		if (_tailer == node)
		{
			assert(node->_next == nullptr);
			_tailer = nullptr;
		}

		node->_prev = nullptr;
		node->_next = nullptr;

#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
		_count.fetch_sub(1, std::memory_order_acq_rel);
#endif

		return node;
	}
}