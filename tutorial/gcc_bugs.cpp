/*
g++ --version:

g++ (Ubuntu 10 - 20200416 - 0ubuntu1~18.04) 10.0.1 20200416 (experimental)[master revision 3c3f12e2a76:dcee354ce56:44b326839d864fc10c459916abcc97f35a9ac3de]
Copyright(C) 2020 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "librf.h"
using namespace resumef;

#define GCC_FIX_BUGS	1

static future_t<> gcc_bugs_if_await(event_t e)
{
#if GCC_FIX_BUGS
	auto result = co_await e.wait();
	if (result == false)
#else
	if (co_await e.wait() == false)	//internal compiler error: in fold_convert_loc, at fold-const.c:2435
#endif
		std::cout << "time out!" << std::endl;
	else
		std::cout << "event signal!" << std::endl;
}


static future_t<> gcc_bugs_while_await(mutex_t lock)
{
#if GCC_FIX_BUGS
	for (;;)
	{
		auto result = co_await lock.try_lock();
		if (result) break;
	}
#else
	while (!co_await lock.try_lock());	//internal compiler error: in fold_convert_loc, at fold-const.c:2435
#endif
	std::cout << "OK." << std::endl;
}



#if GCC_FIX_BUGS
static future_t<> gcc_bugs_lambda_coroutines_fixed(std::thread& other, channel_t<bool> c_done)
{
	co_await c_done;
	std::cout << "other thread = " << other.get_id();
	co_await c_done;
}
#endif
static void gcc_bugs_lambda_coroutines()
{
	channel_t<bool> c_done{ 1 };
	std::thread other;

#if GCC_FIX_BUGS
	go gcc_bugs_lambda_coroutines_fixed(other, c_done);
#else
	go[&other, c_done]()->future_t<>
	{
		co_await c_done;
		std::cout << "other thread = " << other.get_id();
		co_await c_done;
	}; //internal compiler error: in captures_temporary, at cp/coroutines.cc:2716
#endif
}



#if GCC_FIX_BUGS
static future_t<> gcc_bugs_lambda_coroutines2_fixed(channel_t<intptr_t> head, channel_t<intptr_t> tail)
{
	for (int i = 0; i < 100; ++i)
	{
		co_await(head << 0);
		co_await tail;
	}
}
#endif
static void gcc_bugs_lambda_coroutines2()
{
	channel_t<intptr_t> head{ 1 };
	channel_t<intptr_t> tail{ 0 };

#if GCC_FIX_BUGS
	go gcc_bugs_lambda_coroutines2_fixed(head, tail);
#else
	GO
	{
		for (int i = 0; i < 100; ++i)
		{
			co_await(head << 0);
			intptr_t value = co_await tail;
		}
	};	//internal compiler error: in captures_temporary, at cp/coroutines.cc:2716
#endif
}



template<class... _Mtxs>
static future_t<> gcc_bugs_nameless_args(adopt_manual_unlock_t
#if GCC_FIX_BUGS
	nameless
#endif
	, _Mtxs&... mtxs)
{
#if GCC_FIX_BUGS
	(void)nameless;
#endif
	co_await mutex_t::lock(adopt_manual_unlock, mtxs...);
}	//internal compiler error: Segmentation fault




void gcc_bugs()
{
	event_t e;
	go gcc_bugs_if_await(e);

	mutex_t mtx;
	go gcc_bugs_while_await(mtx);

	mutex_t a, b, c;
	go gcc_bugs_nameless_args(adopt_manual_unlock, a, b, c);
}
