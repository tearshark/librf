#pragma once

#include <yvals.h>
#include <atomic>
#include <memory>
#include <chrono>
#include <thread>
#include <list>
#include <vector>
#include <deque>
#include <mutex>
#include <functional>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include <assert.h>

#include <experimental/coroutine>
//#include <experimental/generator>
//替代<experimental/generator>，因为VS2015/VS2017的generator<>未实现return_value，导致yield后不能return
#include "generator.h"		

#if defined(RESUMEF_DLL_EXPORT)
#	define RF_API __declspec(dllexport)
#elif defined(RESUMEF_DLL_IMPORT)
#	define RF_API __declspec(dllimport)
#else
#	define RF_API
#endif

#if RESUMEF_DEBUG_COUNTER
extern std::mutex g_resumef_cout_mutex;
extern std::atomic<intptr_t> g_resumef_state_count;
extern std::atomic<intptr_t> g_resumef_task_count;
extern std::atomic<intptr_t> g_resumef_evtctx_count;
#endif

namespace resumef
{
#if _HAS_CXX17
	template<class... _Mutexes>
	using scoped_lock = std::scoped_lock<_Mutexes...>;
#else
	template<class... _Mutexes>
	using scoped_lock = std::lock_guard<_Mutexes...>;
#endif
	template<typename _PromiseT = void>
	using coroutine_handle = std::experimental::coroutine_handle<_PromiseT>;

	template<typename _PromiseT = void>
	inline void * _coro_function_ptr(coroutine_handle<> coro)
	{
		auto frame_prefix = (coroutine_handle<void>::_Resumable_frame_prefix*)coro.address();
		return reinterpret_cast<void *>(frame_prefix->_Fn);
	}

	template<typename _PromiseT>
	inline _PromiseT * _coro_promise_ptr__(void * _Ptr)
	{
		using coroutine_instance = coroutine_handle<_PromiseT>;
		return reinterpret_cast<_PromiseT *>(reinterpret_cast<char *>(_Ptr) - coroutine_instance::_ALIGNED_SIZE);
	}

#define _coro_promise_ptr(T) _coro_promise_ptr__<resumef::promise_t<T> >(_coro_frame_ptr())

	enum struct future_error
	{
		none,
		not_ready,			// get_value called when value not available
		already_acquired,	// attempt to get another future
		unlock_more,		// unlock 次数多余lock次数
		read_before_write,	// 0容量的channel，先读后写

		max__
	};

	const char * get_error_string(future_error fe, const char * classname);
	//const char * future_error_string[size_t(future_error::max__)];

	struct future_exception : std::exception
	{
		future_error _error;
		future_exception(future_error fe) 
			: exception(get_error_string(fe, "future_exception"))
			, _error(fe)
		{
		}
	};

	struct lock_exception : std::exception
	{
		future_error _error;
		lock_exception(future_error fe)
			: exception(get_error_string(fe, "lock_exception"))
			, _error(fe)
		{
		}
	};

	struct channel_exception : std::exception
	{
		future_error _error;
		channel_exception(future_error fe)
			: exception(get_error_string(fe, "channel_exception"))
			, _error(fe)
		{
		}
	};

	struct scheduler;
	struct state_base;

	//获得当前线程下的调度器
	extern scheduler * this_scheduler();
	//获得当前线程下，正在由调度器调度的协程
	//extern state_base * this_coroutine();

	//namespace detail
	//{
	//	extern state_base * current_coroutine();
	//}
}

#define co_yield_void co_yield nullptr
#define co_return_void co_return nullptr
