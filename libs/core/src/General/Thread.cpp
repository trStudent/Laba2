/**
 * @file Thread.cpp
 * @brief Implementation of the Thread RAII wrapper class.
 * @author Timofei Romanchuck
 * @date 2026-01-03
 */

#include <core/General/Thread.h>

namespace core::General {

    void Thread::set_zero_() noexcept
    {
        hThread_ = nullptr;
        tid_ = INVALID_ID;
    }

    bool Thread::is_valid_handle(HANDLE h) noexcept
    {
        // Windows API is inconsistent: some functions return NULL on failure,
        // while others return INVALID_HANDLE_VALUE (-1). We check for both.
        return nullptr != h && INVALID_HANDLE_VALUE != h;
    }

    void Thread::close_handle_(HANDLE h) noexcept
    {
        if (is_valid_handle(h))
        {
            CloseHandle(h);
        }
    }

    void Thread::initialize_() noexcept
    {
        if (is_valid_handle(hThread_))
        {
            // Ensure the thread ID is synchronized with the handle. 
            // Useful if the handle was obtained via OpenThread without an explicit ID.
            if (INVALID_ID == tid_)
                tid_ = GetThreadId(hThread_);

            // If GetThreadId fails, the handle is likely invalid or lacks permissions.
            if (INVALID_ID == tid_)
                reset();
        }
        else
            set_zero_();
    }

    Thread::Thread() noexcept
        : hThread_(nullptr), tid_(INVALID_ID) { }

    Thread::~Thread()
    {
        // RAII: Ensure the handle is closed when the wrapper goes out of scope.
        reset();
    }

    Thread::Thread(Thread&& other) noexcept
        : hThread_(other.hThread_), tid_(other.tid_)
    {
        // Ownership transfer: source must be nullified to prevent double-closing.
        other.set_zero_();
    }

    Thread& Thread::operator=(Thread&& other) noexcept
    {
        if (&other != this)
        {
            // Close existing resource before taking over the new one.
            reset();
            hThread_ = other.hThread_;
            tid_ = other.tid_;
            other.set_zero_();
        }
        return *this;
    }

    bool Thread::valid() const noexcept
    {
        return is_valid_handle(hThread_);
    }

    Thread::operator bool() const noexcept
    {
        return valid();
    }

    bool Thread::joinable() const noexcept
    {
        return valid();
    }

    size_t Thread::get_id() const noexcept
    {
        return static_cast<size_t>(tid_);
    }

    HANDLE Thread::handle() noexcept
    {
        return hThread_;
    }

    size_t Thread::hardware_concurrency()
    {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        return static_cast<size_t>(sysinfo.dwNumberOfProcessors);
    }

    HANDLE Thread::release() noexcept
    {
        HANDLE temp = hThread_;
        // Clear state without closing the handle, effectively giving it to the caller.
        set_zero_();
        return temp;
    }

    void Thread::reset() noexcept
    {
        close_handle_(hThread_);
        set_zero_();
    }

    void Thread::reset(HANDLE thread_handle, DWORD thread_id) noexcept
    {
        reset();
        hThread_ = thread_handle;
        tid_ = thread_id;
        initialize_();
    }

    void Thread::swap(Thread& other) noexcept
    {
        HANDLE tempH = hThread_;
        DWORD tempTid = tid_;

        hThread_ = other.hThread_;
        tid_ = other.tid_;

        other.hThread_ = tempH;
        other.tid_ = tempTid;
    }

    void Thread::join() noexcept
    {
        if (valid())
        {
            // Block the caller until the kernel object (thread) becomes signaled.
            WaitForSingleObject(hThread_, INFINITE);
            // Post-condition: clean up the handle since the thread has terminated.
            reset();
        }
    }

    void Thread::detach() noexcept
    {
        // Simply close the handle reference. The OS keeps the thread alive 
        // until it finishes, but we can no longer control it.
        reset();
    }

    std::optional<DWORD> Thread::try_exit_code() const noexcept
    {
        if (!valid())
            return std::nullopt;

        DWORD exitCode = 0;
        if (GetExitCodeThread(hThread_, &exitCode))
        {
            // Windows uses 259 (STILL_ACTIVE) as a special status.
            // If the thread actually returns 259, it is indistinguishable from 'running'.
            if (STILL_ACTIVE == exitCode)
                return std::nullopt;
            return exitCode;
        }
        return std::nullopt;
    }

    bool Thread::is_running() const noexcept
    {
        if (!valid()) return false;

        DWORD exitCode = 0;
        if (GetExitCodeThread(hThread_, &exitCode))
            return (STILL_ACTIVE == exitCode);
        return false;
    }

    bool Thread::terminate(UINT exit_code) noexcept
    {
        if (valid())
            // Warning: TerminateThread is dangerous as it does not clean up 
            // thread stacks or release locks held by the thread.
            return 0 != TerminateThread(hThread_, exit_code);
        return false;
    }

    bool Thread::suspend() noexcept
    {
        if (valid())
            // SuspendThread increments the suspend count.
            return ERROR_STATUS != SuspendThread(hThread_);
        return false;
    }

    bool Thread::resume() noexcept
    {
        if (valid())
            // ResumeThread decrements the suspend count; thread runs if count reaches 0.
            return ERROR_STATUS != ResumeThread(hThread_);
        return false;
    }

    wait_status Thread::wait() noexcept
    {
        if (valid())
        {
            DWORD result = WaitForSingleObject(hThread_, INFINITE);
            return static_cast<wait_status>(result);
        }
        return wait_status::failed;
    }

    wait_status Thread::wait_for(milliseconds timeout) noexcept
    {
        if (valid())
        {
            auto ms_count = timeout.count();
            // Clamping the value to MAX_WAIT_TIMEOUT prevents a high value from 
            // being interpreted as INFINITE (0xFFFFFFFF) by the kernel.
            DWORD ms = (MAX_WAIT_TIMEOUT < ms_count) ? (MAX_WAIT_TIMEOUT) : static_cast<DWORD>(ms_count);

            DWORD result = WaitForSingleObject(hThread_, ms);
            return static_cast<wait_status>(result);
        }
        return wait_status::failed;
    }

    bool Thread::set_priority(DWORD priority) noexcept
    {
        if (valid())
            // Priority is relative to the process priority class.
            return 0 != SetThreadPriority(hThread_, static_cast<int>(priority));
        return false;
    }

    DWORD Thread::get_priority() const noexcept
    {
        if (valid())
        {
            int p = GetThreadPriority(hThread_);
            if (THREAD_PRIORITY_ERROR_RETURN != p)
                return static_cast<DWORD>(p);
        }
        return ERROR_PRIORITY;
    }

    DWORD_PTR Thread::set_affinity(DWORD_PTR mask) noexcept
    {
        if (valid())
            // Restricts the thread to run on specific logical processors defined by the bitmask.
            return SetThreadAffinityMask(hThread_, mask);
        return 0;
    }

    Thread Thread::create(
        LPSECURITY_ATTRIBUTES lpThreadAttributes,
        DWORD dwStackSize,
        LPTHREAD_START_ROUTINE lpStartAddress,
        LPVOID lpParameter,
        DWORD dwCreationFlags,
        LPDWORD lpThreadId) noexcept
    {
        DWORD tid = INVALID_ID;
        // Use the native Win32 function to spawn a kernel-level thread.
        HANDLE h = CreateThread(
            lpThreadAttributes,
            dwStackSize,
            lpStartAddress,
            lpParameter,
            dwCreationFlags,
            &tid
        );

        if (h)
        {
            Thread t;
            // Optionally return the ID to the caller if a pointer was provided.
            if (nullptr != lpThreadId)
                *lpThreadId = tid;
            // Transfer ownership to the RAII wrapper.
            t.reset(h, tid);
            return t;
        }

        return Thread();
    }

    void swap(Thread& a, Thread& b) noexcept
    {
        a.swap(b);
    }

} // namespace core::General