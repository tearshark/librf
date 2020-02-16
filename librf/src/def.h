#pragma once

#define RESUMEF_LIB_INCLUDE	1

//#include <yvals.h>
#include <atomic>
#include <chrono>
#include <vector>
#include <deque>
#include <mutex>
#include <map>
#include <optional>

#include <assert.h>

#include <experimental/coroutine>
//#include <experimental/generator>
//替代<experimental/generator>，因为VS2015/VS2017的generator<>未实现return_value，导致yield后不能return
#include "generator.h"		

namespace resumef
{
	struct scheduler_t;

	template<class _Ty = void>
	struct future_t;
	using future_vt [[deprecated]] = future_t<>;

	template<class _Ty = void>
	struct promise_t;

	template<class _Ty = void>
	struct awaitable_t;

	//获得当前线程下的调度器
	scheduler_t* this_scheduler();

	template <typename _Ty = std::nullptr_t, typename _Alloc = std::allocator<char>>
	using generator_t = std::experimental::generator<_Ty, _Alloc>;

	template<typename _PromiseT = void>
	using coroutine_handle = std::experimental::coroutine_handle<_PromiseT>;

	struct state_base_t;

#if _HAS_CXX17
	template<class... _Mutexes>
	using scoped_lock = std::scoped_lock<_Mutexes...>;
#else
	template<class... _Mutexes>
	using scoped_lock = std::lock_guard<_Mutexes...>;
#endif
}

#if RESUMEF_DEBUG_COUNTER
extern std::mutex g_resumef_cout_mutex;
extern std::atomic<intptr_t> g_resumef_state_count;
extern std::atomic<intptr_t> g_resumef_task_count;
extern std::atomic<intptr_t> g_resumef_evtctx_count;
extern std::atomic<intptr_t> g_resumef_state_id;
#endif

namespace std
{
#if !_HAS_CXX20
	template<class T>
	struct remove_cvref
	{
		typedef std::remove_cv_t<std::remove_reference_t<T>> type;
	};
#if _HAS_CXX17
	template<class T>
	using remove_cvref_t = typename std::remove_cvref<T>::type;
#endif
#endif
}

#if defined(RESUMEF_DLL_EXPORT)
#	define RF_API __declspec(dllexport)
#elif defined(RESUMEF_DLL_IMPORT)
#	define RF_API __declspec(dllimport)
#else
#	define RF_API
#endif

#include "exception.inl"
#include "type_traits.inl"

#define co_yield_void co_yield nullptr
#define co_return_void co_return nullptr
