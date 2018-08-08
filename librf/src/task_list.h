#pragma once

namespace resumef
{
	struct task_list
	{
		using value_type = task_base;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type *;
		using const_pointer = const value_type *;
		using reference = value_type & ;
		using const_reference = const value_type&;

		//using iterator = typename _Mybase::iterator;
		//using const_iterator = typename _Mybase::const_iterator;
		//using reverse_iterator = std::reverse_iterator<iterator>;
		//using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		
	private:
		pointer			_M_header;
		pointer			_M_last;
	public:
		task_list()
			: _M_header(nullptr)
			, _M_last(nullptr)
		{
		}

		~task_list()
		{
			clear(true);
		}

		task_list(const task_list& _Right) = delete;
		task_list& operator=(const task_list& _Right) = delete;

		task_list(task_list&& _Right)
		{
			_M_header = _Right._M_header;
			_Right._M_header = nullptr;
			_M_last = _Right._M_last;
			_Right._M_last = nullptr;
		}

		task_list& operator=(task_list&& _Right)
		{	// assign by moving _Right
			if (this != std::addressof(_Right))
			{	// different, assign it
				clear(true);
				_Move_assign(_Right);
			}
			return (*this);
		}

		void clear(bool cancel_)
		{
			pointer header = _M_header;
			_M_header = _M_last = nullptr;

			for (; header; )
			{
				pointer temp = header;
				header = header->_next_node;

				if (cancel_)
					temp->cancel();
				delete temp;
			}
		}

		void push_back(pointer node)
		{
			assert(node != nullptr);
			assert(node->_next_node == nullptr);
			assert(node->_prev_node == nullptr);
			_Push_back(node);
		}

		pointer erase(pointer node, bool cancel_)
		{
			assert(node != nullptr);

			pointer const next = node->_next_node;
			pointer const prev = node->_prev_node;
			if (next)
				next->_prev_node = prev;
			if (prev)
				prev->_next_node = next;

			if (_M_header == node)
				_M_header = next;
			if (_M_last == node)
				_M_last = prev;

			if (cancel_)
				node->cancel();
			delete node;

			return next;
		}

		void merge_back(task_list & _Right)
		{
			if (this == std::addressof(_Right) || _Right._M_header == nullptr)
				return;

			if (_M_header == nullptr)
			{
				_Move_assign(_Right);
			}
			else
			{
				assert(_M_last != nullptr);

				_M_last->_next_node = _Right._M_header;
				_Right._M_header->_prev_node = _M_last;
				_M_last = _Right._M_last;

				_Right._M_header = _Right._M_last = nullptr;
			}
		}

		bool empty() const
		{
			return _M_header == nullptr;
		}

		pointer begin() const
		{
			return _M_header;
		}

		pointer end() const
		{
			return nullptr;
		}

	private:
		void _Push_back(pointer node)
		{
			if (_M_header == nullptr)
			{
				assert(_M_last == nullptr);
				_M_header = _M_last = node;
			}
			else
			{
				assert(_M_last != nullptr);
				_M_last->_next_node = node;
				node->_prev_node = _M_last;

				_M_last = node;
			}
		}

		void _Move_assign(task_list& _Right)
		{
			assert(this != std::addressof(_Right));
				
			_M_header = _Right._M_header;
			_Right._M_header = nullptr;
			_M_last = _Right._M_last;
			_Right._M_last = nullptr;
		}
	};

}
