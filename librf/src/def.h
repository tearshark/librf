#pragma once

#define LIB_RESUMEF_VERSION 20301 // 2.3.1

#if defined(RESUMEF_MODULE_EXPORT)
#define RESUMEF_NS export namespace resumef
#else
#define RESUMEF_NS namespace resumef
#endif

//如果不清楚context frame的内存布局的情况下，该值设置为0
#if defined(__clang__) || defined(_MSC_VER)
#define RESUMEF_INLINE_STATE	1
#else
#define RESUMEF_INLINE_STATE	0
#endif

RESUMEF_NS
{
	struct scheduler_t;

	template<class _Ty = void>
	struct future_t;

	using future_vt [[deprecated]] = future_t<>;

	template <typename _Ty = std::nullptr_t, typename _Alloc = std::allocator<char>>
	struct generator_t;

	template<class _Ty = void>
	struct promise_t;

	template<class _Ty = void>
	struct awaitable_t;

	//获得当前线程下的调度器
	scheduler_t* this_scheduler();

	template<typename _PromiseT = void>
	using coroutine_handle = std::experimental::coroutine_handle<_PromiseT>;

	struct state_base_t;

	struct switch_scheduler_t;

	template<class... _Mutexes>
	using scoped_lock = std::scoped_lock<_Mutexes...>;

	constexpr size_t _Version = LIB_RESUMEF_VERSION;
}

#if RESUMEF_DEBUG_COUNTER
extern std::mutex g_resumef_cout_mutex;
extern std::atomic<intptr_t> g_resumef_state_count;
extern std::atomic<intptr_t> g_resumef_task_count;
extern std::atomic<intptr_t> g_resumef_evtctx_count;
extern std::atomic<intptr_t> g_resumef_state_id;
#endif

RESUMEF_NS
{
	template<class T>
	struct remove_cvref
	{
		typedef std::remove_cv_t<std::remove_reference_t<T>> type;
	};
	template<class T>
	using remove_cvref_t = typename remove_cvref<T>::type;


	template<class _Ty>
	constexpr size_t _Align_size()
	{
		const size_t _ALIGN_REQ = sizeof(void*) * 2;
		return std::is_empty_v<_Ty> ? 0 :
			(sizeof(_Ty) + _ALIGN_REQ - 1) & ~(_ALIGN_REQ - 1);
	}
}

#include "exception.inl"
#include "type_traits.inl"
#include "macro_def.inl"
