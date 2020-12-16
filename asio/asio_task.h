#pragma once

#if ASIO_VERSION >= 101202
#include "asio_task_1.12.2.inl"
#elif ASIO_VERSION >= 101200
#include "asio_task_1.12.0.inl"
#else
#include "asio_task_1.10.0.inl"
#endif
