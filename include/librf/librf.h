/*
 *Copyright 2017~2020 lanzhengpeng
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <atomic>
#include <chrono>
#include <array>
#include <vector>
#include <deque>
#include <mutex>
#include <map>
#include <list>
#include <any>
#include <unordered_map>
#include <functional>
#include <optional>
#include <thread>
#include <cassert>
#include <utility>

#if __cpp_impl_coroutine
#include <coroutine>
#ifdef _MSC_VER
#ifndef __clang__
extern "C" size_t _coro_frame_size();
extern "C" void* _coro_frame_ptr();
extern "C" void _coro_init_block();
extern "C" void* _coro_resume_addr();
extern "C" void _coro_init_frame(void*);
extern "C" void _coro_save(size_t);
extern "C" void _coro_suspend(size_t);
extern "C" void _coro_cancel();
extern "C" void _coro_resume_block();

#pragma intrinsic(_coro_frame_size)
#pragma intrinsic(_coro_frame_ptr)
#pragma intrinsic(_coro_init_block)
#pragma intrinsic(_coro_resume_addr)
#pragma intrinsic(_coro_init_frame)
#pragma intrinsic(_coro_save)
#pragma intrinsic(_coro_suspend)
#pragma intrinsic(_coro_cancel)
#pragma intrinsic(_coro_resume_block)
#else
#include "src/unix/clang_builtin.h"
#endif
#endif
#elif defined(__clang__)
#include "src/unix/coroutine.h"
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#else
#include "src/unix/coroutine.h"
#endif

#include "src/stop_token.h"

#include "src/config.h"

#if RESUMEF_DEBUG_COUNTER
#include <iostream>
#endif

#include "src/def.h"
#include "src/macro_def.inl"
#include "src/exception.inl"
#include "src/counted_ptr.h"
#include "src/type_traits.inl"
#include "src/type_concept.inl"
#include "src/spinlock.h"
#include "src/state.h"
#include "src/future.h"
#include "src/promise.h"
#include "src/awaitable.h"
#include "src/generator.h"

#include "src/rf_task.h"
#include "src/timer.h"
#include "src/scheduler.h"

#include "src/promise.inl"
#include "src/state.inl"

#include "src/switch_scheduler.h"
#include "src/current_scheduler.h"

#include "src/yield.h"
#include "src/sleep.h"
#include "src/when.h"

#include "src/_awaker.h"
#include "src/ring_queue.h"
#include "src/intrusive_link_queue.h"
#include "src/channel.h"
#include "src/event.h"
#include "src/mutex.h"

namespace resumef = librf;
