﻿#pragma once

#ifndef _offset_of
#define _offset_of(c, m) reinterpret_cast<size_t>(&static_cast<c *>(0)->m)
#endif

#define co_yield_void co_yield nullptr
#define co_return_void co_return nullptr

#if !defined(_DISABLE_RESUMEF_GO_MACRO)
#define go (*::librf::this_scheduler()) + 
#define GO (*::librf::this_scheduler()) + [=]()mutable->librf::future_t<>
#endif

#define librf_current_scheduler() (co_await ::librf::get_current_scheduler())
#define librf_root_state() (co_await ::librf::get_root_state())
#define librf_current_task() (co_await ::librf::get_current_task())

#if defined(__clang__) || defined(__GNUC__)
#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif // likely
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif // unlikely
#else  // defined(__clang__) || defined(__GNUC__)
#ifndef likely
#define likely(x) x
#endif // likely
#ifndef unlikely
#define unlikely(x) x
#endif // unlikely
#endif // defined(__clang__) || defined(__GNUC__)

#ifndef LIBRF_USE_STATIC_LIBRARY
#  if _WIN32
#    ifdef LIBRF_DYNAMIC_EXPORTS
#      define LIBRF_API __declspec(dllexport)
#    else //RESUMEF_DYNAMIC_EXPORTS
#      define LIBRF_API __declspec(dllimport)
#    endif //RESUMEF_DYNAMIC_EXPORTS
#  else //_WIN32
#    define LIBRF_API __attribute__((visibility("default")))
#  endif //_WIN32
#else //RESUMEF_USE_SHARD_LIBRARY
#  define LIBRF_API
#endif //RESUMEF_USE_SHARD_LIBRARY
