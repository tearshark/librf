
#pragma once

RESUMEF_NS
{
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

	template <typename T, typename... Args>
	counted_ptr<T> make_counted(Args&&... args)
	{
		return new T{std::forward<Args>(args)...};
	}
}

