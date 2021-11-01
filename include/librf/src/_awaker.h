#pragma once

#ifndef DOXYGEN_SKIP_PROPERTY

namespace librf
{
	namespace detail
	{
		template<class _Ety, class... _Types>
		struct _awaker
		{
			//如果超时
			//		e 始终为nullptr
			//		不关心返回值
			//如果不是超时，
			//		e 指向当前触发的事件，用于实现wait_any
			//		返回true表示成功触发了事件，event内部减小一次事件计数，并删除此awaker
			//		返回false表示此事件已经无效，event内部只删除此awaker
			typedef std::function<bool(_Ety * e, _Types...)> callee_type;
		private:
			typedef spinlock lock_type;
			//typedef std::recursive_mutex lock_type;

			lock_type _lock;
			callee_type _callee;
			std::atomic<intptr_t> _counter;
		public:
			_awaker(callee_type && callee_, intptr_t init_count_ = 0)
				: _callee(std::forward<callee_type>(callee_))
				, _counter(init_count_)
			{
			}

			//调用一次后，_callee就被置nullptr，下次再调用，必然返回false
			//第一次调用，返回调用_callee的返回值
			//超时通过传入nullptr来调用
			bool awake(_Ety * e, intptr_t count_, const _Types&... args)
			{
				assert(count_ > 0);
				scoped_lock<lock_type> lock_(this->_lock);

				if ((this->_counter.fetch_sub(count_) - count_) <= 0)
				{
					if (this->_callee)
					{
						callee_type callee_ = std::move(this->_callee);
						return callee_(e, args...);
					}
					return false;
				}
				return true;
			}

		private:
			_awaker(const _awaker &) = delete;
			_awaker(_awaker &&) = delete;
			_awaker & operator = (const _awaker &) = delete;
			_awaker & operator = (_awaker &&) = delete;
		};
	}
}

#endif	//DOXYGEN_SKIP_PROPERTY
 