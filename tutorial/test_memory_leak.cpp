
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "librf/librf.h"

using namespace librf;
using namespace std::chrono;

void test_memory_leak()
{
    go[]()->future_t<void>
    {
        event_t e;
        for (;;)
        {
            bool val = co_await e.wait_for(1ms);
            assert(val == false);
            co_await yield();
        }
    };

    for (;;)
    {
        auto have = this_scheduler()->run_one_batch();
        if (!have) {
            std::this_thread::sleep_for(1ms);
        }
    }
}

#if LIBRF_TUTORIAL_STAND_ALONE
int main()
{
    test_memory_leak();
    return 0;
}
#endif
