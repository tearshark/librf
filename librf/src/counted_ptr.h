#pragma once

#ifndef DOXYGEN_SKIP_PROPERTY
RESUMEF_NS
{
#endif	//DOXYGEN_SKIP_PROPERTY

	template <typename T>
	struct counted_ptr
	{
		counted_ptr() noexcept = default;
		counted_ptr(const counted_ptr& cp) : _p(cp._p) 
		{
			_lock();
		}

		counted_ptr(T* p) : _p(p) 
		{
			_lock();
		}

		counted_ptr(counted_ptr&& cp) noexcept
		{
			std::swap(_p, cp._p);
		}

		counted_ptr& operator=(const counted_ptr& cp) 
		{
			if (&cp != this)
			{
				counted_ptr t = cp;
				std::swap(_p, t._p);
			}
			return *this;
		}

		counted_ptr& operator=(counted_ptr&& cp) noexcept
		{
			if (&cp != this)
				std::swap(_p, cp._p);
			return *this;
		}

		~counted_ptr()
		{
			_unlock();
		}

		T* operator->() const noexcept
		{
			return _p;
		}

		T* get() const noexcept
		{
			return _p;
		}

		void reset()
		{
			_unlock();
		}
	private:
		void _unlock()
		{
			if (_p != nullptr)
			{
				auto t = _p;
				_p = nullptr;
				t->unlock();
			}
		}
		void _lock(T* p)
		{
			if (p != nullptr)
				p->lock();
			_p = p;
		}
		void _lock()
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

