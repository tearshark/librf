#pragma once

#if 0

extern "C" void  __builtin_coro_resume(void* addr);
extern "C" void  __builtin_coro_destroy(void* addr);
extern "C" bool  __builtin_coro_done(void* addr);
extern "C" void* __builtin_coro_promise(void* addr, const long unsigned int alignment, bool from_promise);
#pragma intrinsic(__builtin_coro_resume)
#pragma intrinsic(__builtin_coro_destroy)
#pragma intrinsic(__builtin_coro_done)
#pragma intrinsic(__builtin_coro_promise)

#endif
