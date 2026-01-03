#ifndef TYPE_H
#define TYPE_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif


#include <chrono>
#include <Windows.h>

namespace core::General
{
    typedef std::chrono::milliseconds milliseconds;

    enum class wait_status : DWORD {
        signaled    = WAIT_OBJECT_0,
        timeout     = WAIT_TIMEOUT,
        failed      = WAIT_FAILED,
        abandoned   = WAIT_ABANDONED
    };
} // namespace core::General

#endif // TYPE_H