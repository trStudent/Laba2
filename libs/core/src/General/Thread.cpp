#include <core/General/Thread.h>

namespace core::General {

    void Thread::set_zero_() noexcept
    {
        hThread_ = nullptr;
        tid_ = 0;
    }

    bool Thread::is_valid_handle(HANDLE h) noexcept
    {
        return h != nullptr && h != INVALID_HANDLE_VALUE;
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
            if (tid_ == 0)
                tid_ = GetThreadId(hThread_);

            if (tid_ == 0)
                reset();
        }
        else
            set_zero_();
    }

    Thread::Thread() noexcept
        : hThread_(nullptr), tid_(0) { }

    Thread::~Thread()
    {
        reset();
    }

    Thread::Thread(Thread&& _other) noexcept
        : hThread_(_other.hThread_), tid_(_other.tid_)
    {
        _other.set_zero_();
    }

    Thread& Thread::operator=(Thread&& other) noexcept
    {
        if (this != &other)
        {
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

    void Thread::swap(Thread& other_) noexcept
    {
        HANDLE tempH = hThread_;
        DWORD tempTid = tid_;

        hThread_ = other_.hThread_;
        tid_ = other_.tid_;

        other_.hThread_ = tempH;
        other_.tid_ = tempTid;
    }

    void Thread::join() noexcept
    {
        if (valid())
        {
            WaitForSingleObject(hThread_, INFINITE);
            reset();
        }
    }

    void Thread::detach() noexcept
    {
        reset();
    }

    std::optional<DWORD> Thread::try_exit_code() const noexcept
    {
        if (!valid())
            return std::nullopt;

        DWORD exitCode = 0;
        if (GetExitCodeThread(hThread_, &exitCode))
        {
            if (exitCode == STILL_ACTIVE)
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
            return (exitCode == STILL_ACTIVE);
        return false;
    }

    bool Thread::terminate(UINT exit_code) noexcept
    {
        if (valid())
            return TerminateThread(hThread_, exit_code) != 0;
        return false;
    }

    bool Thread::suspend() noexcept
    {
        if (valid())
            return SuspendThread(hThread_) != (DWORD)-1;
        return false;
    }

    bool Thread::resume() noexcept
    {
        if (valid())
            return ResumeThread(hThread_) != (DWORD)-1;
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
            DWORD ms = (ms_count >= INFINITE) ? (INFINITE - 1) : static_cast<DWORD>(ms_count);

            DWORD result = WaitForSingleObject(hThread_, ms);
            return static_cast<wait_status>(result);
        }
        return wait_status::failed;
    }

    bool Thread::set_priority(DWORD priority) noexcept
    {
        if (valid())
            return SetThreadPriority(hThread_, static_cast<int>(priority)) != 0;
        return false;
    }

    DWORD Thread::get_priority() const noexcept
    {
        if (valid())
        {
            int p = GetThreadPriority(hThread_);
            if (p != THREAD_PRIORITY_ERROR_RETURN)
                return static_cast<DWORD>(p);
        }
        return 0;
    }

    DWORD_PTR Thread::set_affinity(DWORD_PTR mask) noexcept
    {
        if (valid())
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
        DWORD tid = 0;
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
            if (lpThreadId)
                *lpThreadId = tid;
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