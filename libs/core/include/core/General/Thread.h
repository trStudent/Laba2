#ifndef THREAD_H
#define THREAD_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <chrono>
#include <windows.h>
#include <optional>

namespace core::General
{
    typedef std::chrono::milliseconds milliseconds;

    enum class wait_status : DWORD {
        signaled    = WAIT_OBJECT_0,
        timeout     = WAIT_TIMEOUT,
        failed      = WAIT_FAILED,
        abandoned   = WAIT_ABANDONED
    };

    class Thread
    {
        private:
            HANDLE hThread_;
            DWORD tid_;
        public:
            Thread() noexcept;
            ~Thread();

            Thread(Thread&& _other) noexcept;
            Thread& operator=(Thread&& other) noexcept;
            
            Thread& operator=(const Thread& other) = delete;
            Thread(const Thread& _other) = delete;

            bool valid() const noexcept;
            operator bool() const noexcept;

            void join() noexcept;
            void detach() noexcept;

            bool joinable() const noexcept;
            size_t get_id() const noexcept;

            HANDLE handle() noexcept;
            static size_t hardware_concurrency();

            HANDLE release() noexcept;
            void reset() noexcept;
            void reset(
                HANDLE thread_handle,
                DWORD thread_id = 0) noexcept;

                
            void swap(Thread& other_) noexcept;
            
            std::optional<DWORD> try_exit_code() const noexcept;
            bool is_running() const noexcept;
            bool terminate(UINT exit_code = 0) noexcept;

            bool suspend() noexcept;
            bool resume() noexcept;

            wait_status wait() noexcept;
            wait_status wait_for(milliseconds timeout) noexcept;

            bool set_priority(DWORD priority) noexcept;
            DWORD get_priority() const noexcept;

            DWORD_PTR set_affinity(DWORD_PTR mask) noexcept;

            static Thread create(
                LPSECURITY_ATTRIBUTES lpThreadAttributes, 
                DWORD dwStackSize,
                LPTHREAD_START_ROUTINE lpStartAddress,
                LPVOID lpParameter,
                DWORD dwCreationFlags,
                LPDWORD lpThreadId) noexcept;

        private:
            static void close_handle_(HANDLE h) noexcept;
            void initialize_() noexcept;
            void set_zero_() noexcept;
            static bool is_valid_handle(HANDLE h) noexcept;
    };

    
    extern void swap(Thread& a, Thread& b) noexcept;

} // namespace core::General


#endif // THREAD_H