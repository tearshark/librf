#pragma once

//BUILTIN(__builtin_coro_resume, "vv*", "")
//BUILTIN(__builtin_coro_destroy, "vv*", "")
//BUILTIN(__builtin_coro_done, "bv*", "n")
//BUILTIN(__builtin_coro_promise, "v*v*IiIb", "n")
//
//BUILTIN(__builtin_coro_size, "z", "n")
//BUILTIN(__builtin_coro_frame, "v*", "n")
//BUILTIN(__builtin_coro_noop, "v*", "n")
//BUILTIN(__builtin_coro_free, "v*v*", "n")
//
//BUILTIN(__builtin_coro_id, "v*Iiv*v*v*", "n")
//BUILTIN(__builtin_coro_alloc, "b", "n")
//BUILTIN(__builtin_coro_begin, "v*v*", "n")
//BUILTIN(__builtin_coro_end, "bv*Ib", "n")
//BUILTIN(__builtin_coro_suspend, "cIb", "n")
//BUILTIN(__builtin_coro_param, "bv*v*", "n")

extern "C" void  __builtin_coro_resume(void* addr);
extern "C" void  __builtin_coro_destroy(void* addr);
extern "C" bool  __builtin_coro_done(void* addr);
extern "C" void* __builtin_coro_promise(void* addr, int alignment, bool from_promise);
#pragma intrinsic(__builtin_coro_resume)
#pragma intrinsic(__builtin_coro_destroy)
#pragma intrinsic(__builtin_coro_done)
#pragma intrinsic(__builtin_coro_promise)

extern "C" size_t __builtin_coro_size();
extern "C" void* __builtin_coro_frame();
extern "C" void* __builtin_coro_noop();
extern "C" void* __builtin_coro_free(void* coro_frame);
#pragma intrinsic(__builtin_coro_size)
#pragma intrinsic(__builtin_coro_frame)
#pragma intrinsic(__builtin_coro_noop)
#pragma intrinsic(__builtin_coro_free)

extern "C" void* __builtin_coro_id(int align, void* promise, void* fnaddr, void* parts);
extern "C" bool  __builtin_coro_alloc();
//extern "C" void* __builtin_coro_begin(void* memory);
//extern "C" void* __builtin_coro_end(void* coro_frame, bool unwind);
extern "C" char  __builtin_coro_suspend(bool final);
//extern "C" bool  __builtin_coro_param(void* original, void* copy);
#pragma intrinsic(__builtin_coro_id)
#pragma intrinsic(__builtin_coro_alloc)
//#pragma intrinsic(__builtin_coro_begin)
//#pragma intrinsic(__builtin_coro_end)
#pragma intrinsic(__builtin_coro_suspend)
//#pragma intrinsic(__builtin_coro_param)

#ifdef __clang__
#define _coro_frame_size() __builtin_coro_size()
#define _coro_frame_ptr() __builtin_coro_frame()
#endif