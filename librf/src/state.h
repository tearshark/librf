
#pragma once

#include "def.h"
#include "spinlock.h"
#include "counted_ptr.h"
#include <iostream>

namespace resumef
{
	struct state_base_t : public counted_t<state_base_t>
	{
		typedef std::recursive_mutex lock_type;

		lock_type	_mtx;
		scheduler_t * _scheduler = nullptr;
		coroutine_handle<> _coro;
		std::exception_ptr _exception;

		RF_API virtual ~state_base_t();
		virtual bool has_value() const = 0;

		void resume()
		{
			scoped_lock<lock_type> __guard(_mtx);

			auto handler = _coro;
			_coro = nullptr;
			handler();
		}
	};

	template <typename _Ty>
	struct state_t : public state_base_t
	{
		using value_type = _Ty;

		std::optional<value_type> _value;
		virtual bool has_value() const override
		{
			return _value.has_value();
		}
	};

	template<>
	struct state_t<void> : public state_base_t
	{
		bool _has_value = false;
		virtual bool has_value() const override
		{
			return _has_value;
		}
	};
}

