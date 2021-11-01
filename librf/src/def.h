#pragma once

#define LIB_RESUMEF_VERSION 30000 // 3.0.0

#ifndef __cpp_impl_coroutine
namespace std
{
	using experimental::coroutine_traits;
	using experimental::coroutine_handle;
	using experimental::suspend_if;
	using experimental::suspend_always;
	using experimental::suspend_never;
}
#endif

namespace resumef
{
#ifndef DOXYGEN_SKIP_PROPERTY
	struct scheduler_t;

	template<class _Ty = void>
	struct future_t;

	template <typename _Ty = std::nullptr_t, typename _Alloc = std::allocator<char>>
	struct generator_t;

	template<class _Ty = void>
	struct promise_t;

	template<class _Ty = void>
	struct awaitable_t;

	struct state_base_t;

	struct switch_scheduler_t;
#endif	//DOXYGEN_SKIP_PROPERTY

	template<typename _PromiseT = void>
	using coroutine_handle = std::coroutine_handle<_PromiseT>;
	using suspend_always = std::suspend_always;
	using suspend_never = std::suspend_never;

	template<class... _Mutexes>
	using scoped_lock = std::scoped_lock<_Mutexes...>;

	using stop_source = milk::concurrency::stop_source;
	using stop_token = milk::concurrency::stop_token;
	template<typename Callback>
	using stop_callback = milk::concurrency::stop_callback<Callback>;
	using milk::concurrency::nostopstate;

	/**
	 * @brief 版本号。
	 */
	constexpr size_t _Version = LIB_RESUMEF_VERSION;

	/**
	 * @brief 获得当前线程下的调度器。
	 */
	scheduler_t* this_scheduler();

}

#ifndef DOXYGEN_SKIP_PROPERTY

#if RESUMEF_DEBUG_COUNTER
extern std::mutex g_resumef_cout_mutex;
extern std::atomic<intptr_t> g_resumef_state_count;
extern std::atomic<intptr_t> g_resumef_task_count;
extern std::atomic<intptr_t> g_resumef_evtctx_count;
extern std::atomic<intptr_t> g_resumef_state_id;
#endif

namespace resumef
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

	template<class _Callable>
	auto make_stop_callback(const stop_token& token, _Callable&& cb) ->std::unique_ptr<stop_callback<_Callable>>
	{
		return std::make_unique<stop_callback<_Callable>>(token, cb);
	}
	template<class _Callable>
	auto make_stop_callback(stop_token&& token, _Callable&& cb) ->std::unique_ptr<stop_callback<_Callable>>
	{
		return std::make_unique<stop_callback<_Callable>>(std::move(token), cb);
	}
}

#include "exception.inl"

#endif	//DOXYGEN_SKIP_PROPERTY
