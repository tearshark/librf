#pragma once

namespace resumef
{
	/**
	 * @brief 专用与state的智能计数指针，通过管理state内嵌的引用计数来管理state的生存期。
	 */
	template <typename T>
	struct counted_ptr
	{
		/**
		 * @brief 构造一个无内容的计数指针。
		 */
		counted_ptr() noexcept = default;

		/**
		 * @brief 拷贝构造函数。
		 */
		counted_ptr(const counted_ptr& cp) noexcept : _p(cp._p)
		{
			_lock();
		}

		/**
		 * @brief 通过裸指针构造一个计数指针。
		 */
		counted_ptr(T* p) noexcept : _p(p)
		{
			_lock();
		}

		/**
		 * @brief 移动构造函数。
		 */
		counted_ptr(counted_ptr&& cp) noexcept : _p(std::exchange(cp._p, nullptr))
		{
		}

		/**
		 * @brief 拷贝赋值函数。
		 */
		counted_ptr& operator=(const counted_ptr& cp)
		{
			if (&cp != this)
			{
				counted_ptr t(cp);
				std::swap(_p, t._p);
			}
			return *this;
		}

		/**
		 * @brief 移动赋值函数。
		 */
		counted_ptr& operator=(counted_ptr&& cp)
		{
			if (&cp != this)
			{
				std::swap(_p, cp._p);
				cp._unlock();
			}
			return *this;
		}

		void swap(counted_ptr& cp) noexcept
		{
			std::swap(_p, cp._p);
		}

		/**
		 * @brief 析构函数中自动做一个计数减一操作。计数减为0，则删除state对象。
		 */
		~counted_ptr()
		{
			_unlock();
		}

		/**
		 * @brief 重载指针操作符。
		 */
		T* operator->() const noexcept
		{
			return _p;
		}

		/**
		 * @brief 获得管理的state指针。
		 */
		T* get() const noexcept
		{
			return _p;
		}

		/**
		 * @brief 重置为空指针。
		 */
		void reset()
		{
			_unlock();
		}
	private:
		void _unlock()
		{
			if (likely(_p != nullptr))
			{
				auto t = _p;
				_p = nullptr;
				t->unlock();
			}
		}
		void _lock(T* p) noexcept
		{
			if (p != nullptr)
				p->lock();
			_p = p;
		}
		void _lock() noexcept
		{
			if (_p != nullptr)
				_p->lock();
		}
		T* _p = nullptr;
	};

	template <typename T, typename U>
	inline bool operator == (const counted_ptr<T>& _Left, const counted_ptr<U>& _Right)
	{
		return _Left.get() == _Right.get();
	}
	template <typename T>
	inline bool operator == (const counted_ptr<T>& _Left, std::nullptr_t)
	{
		return _Left.get() == nullptr;
	}
	template <typename T>
	inline bool operator == (std::nullptr_t, const counted_ptr<T>& _Left)
	{
		return _Left.get() == nullptr;
	}
	template <typename T>
	inline bool operator != (const counted_ptr<T>& _Left, std::nullptr_t)
	{
		return _Left.get() != nullptr;
	}
	template <typename T>
	inline bool operator != (std::nullptr_t, const counted_ptr<T>& _Left)
	{
		return _Left.get() != nullptr;
	}
}

namespace std
{
	template<typename T>
	inline void swap(resumef::counted_ptr<T>& a, resumef::counted_ptr<T>& b) noexcept
	{
		a.swap(b);
	}
}
