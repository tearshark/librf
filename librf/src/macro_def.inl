#pragma once

#define co_yield_void co_yield nullptr
#define co_return_void co_return nullptr
#define resumf_guard_lock(lker) (lker).lock(); resumef::scoped_lock<resumef::mutex_t> __resumf_guard##lker##__(std::adopt_lock, (lker))

#if !defined(_DISABLE_RESUMEF_GO_MACRO)
#define go (*::resumef::this_scheduler()) + 
#define GO (*::resumef::this_scheduler()) + [=]()mutable->resumef::future_t<>
#endif
