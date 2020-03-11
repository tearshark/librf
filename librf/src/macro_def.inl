#pragma once

#ifndef _offset_of
#define _offset_of(c, m) reinterpret_cast<size_t>(&static_cast<c *>(0)->m)
#endif

#define co_yield_void co_yield nullptr
#define co_return_void co_return nullptr
#define resumf_guard_lock(lker) (lker).lock(); resumef::scoped_lock<resumef::mutex_t> __resumf_guard##lker##__(std::adopt_lock, (lker))

#if !defined(_DISABLE_RESUMEF_GO_MACRO)
#define go (*::resumef::this_scheduler()) + 
#define GO (*::resumef::this_scheduler()) + [=]()mutable->resumef::future_t<>
#endif

#define current_scheduler() (co_await ::resumef::get_current_scheduler())
