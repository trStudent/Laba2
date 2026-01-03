/**
 * @file Thread.h
 * @brief RAII wrapper for Windows Thread handles.
 * @author Your Name
 * @date 2026-01-03
 */

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
#include "Type.h"

/**
 * @namespace core::General
 * @brief Core utilities for system object management.
 */
namespace core::General
{
    /**
     * @class Thread
     * @brief A move-only RAII wrapper for a Windows thread handle.
     * 
     * This class manages the lifecycle of a single thread. It supports 
     * synchronization, priority adjustment, and affinity settings. 
     * As an RAII object, it will close the thread handle upon destruction 
     * unless the thread is released or detached.
     */
    class Thread
    {
        private:
            HANDLE hThread_; /**< Internal handle to the Windows thread. */
            DWORD tid_;      /**< Unique thread identifier. */

            /** @name Internal Constants 
             *  @{ */
            static constexpr DWORD INVALID_ID = 0;                        /**< ID used for uninitialized threads. */
            static constexpr DWORD ERROR_STATUS = static_cast<DWORD>(-1); /**< Return value for failed Win32 thread calls. */
            static constexpr DWORD ERROR_PRIORITY = 0;                    /**< Fallback value for priority errors. */
            static constexpr DWORD MAX_WAIT_TIMEOUT = INFINITE - 1;       /**< Maximum duration for wait_for. */
            /** @} */

        public:
            /** @name Lifecycle Management
             *  @{ */

            /** @brief Constructs an empty/invalid thread object. */
            Thread() noexcept;

            /** @brief Destructor. Closes the handle via reset(). */
            ~Thread();

            /** @brief Move constructor. Transfers handle ownership from @p other. */
            Thread(Thread&& other) noexcept;

            /** @brief Move assignment. Closes current handle and takes ownership from @p other. */
            Thread& operator=(Thread&& other) noexcept;
            
            /** @brief Copying is deleted to prevent double-closing of handles. */
            Thread& operator=(const Thread& other) = delete;
            /** @brief Copying is deleted to prevent double-closing of handles. */
            Thread(const Thread& other) = delete;

            /**
             * @brief Blocks until the thread finishes execution.
             * @note Closes the handle after completion, making the object invalid.
             */
            void join() noexcept;

            /**
             * @brief Disassociates the handle from this object.
             * @note The thread continues to run, but this object becomes invalid.
             */
            void detach() noexcept;

            /**
             * @brief Transfers ownership of the handle to the caller.
             * @return The raw Win32 thread handle. The caller is responsible for closing it.
             */
            HANDLE release() noexcept;

            /** @brief Closes the current handle and nullifies internal state. */
            void reset() noexcept;

            /**
             * @brief Resets the object with a new raw handle.
             * @param thread_handle New Win32 thread handle.
             * @param thread_id Corresponding thread ID.
             */
            void reset(
                HANDLE thread_handle,
                DWORD thread_id = INVALID_ID) noexcept;

            /** @brief Swaps state with another Thread object. */
            void swap(Thread& other) noexcept;
            /** @} */

            /** @name Status and Inspection
             *  @{ */

            /** @return true if the handle is valid. */
            bool valid() const noexcept;

            /** @brief Explicit check for validity. */
            operator bool() const noexcept;

            /** @return true if the object holds a valid handle and can be joined. */
            bool joinable() const noexcept;

            /** @return The unique thread identifier. */
            size_t get_id() const noexcept;

            /** @return The raw Win32 handle. */
            HANDLE handle() noexcept;

            /** @return The number of logical processors available on the system. */
            static size_t hardware_concurrency();

            /**
             * @brief Non-blocking check for the thread's exit code.
             * @return The exit code, or std::nullopt if the thread is still running or handle is invalid.
             */
            std::optional<DWORD> try_exit_code() const noexcept;

            /** @return true if the thread is currently executing. */
            bool is_running() const noexcept;
            /** @} */

            /** @name Execution Control
             *  @{ */

            /** @brief Forcibly stops the thread. 
             *  @warning unsafe operation. */
            bool terminate(UINT exit_code = 0) noexcept;

            /** @brief Suspends the thread's execution. */
            bool suspend() noexcept;

            /** @brief Resumes a suspended thread. */
            bool resume() noexcept;

            /**
             * @brief Blocks indefinitely until the thread terminates.
             * @return signaled on success, failed otherwise.
             */
            wait_status wait() noexcept;

            /**
             * @brief Blocks for a duration until the thread terminates.
             * @param timeout Duration to wait.
             * @return signaled, timeout, or failed.
             */
            wait_status wait_for(milliseconds timeout) noexcept;

            /** @brief Sets the execution priority of the thread. */
            bool set_priority(DWORD priority) noexcept;

            /** @return The current thread priority or ERROR_PRIORITY on failure. */
            DWORD get_priority() const noexcept;

            /** @brief Restricts the thread to specific logical processors. */
            DWORD_PTR set_affinity(DWORD_PTR mask) noexcept;
            /** @} */

            /** @name Thread Creation
             *  @{ */

            /**
             * @brief Static factory to create and start a new thread.
             * @param lpThreadAttributes Security attributes.
             * @param dwStackSize Initial stack size.
             * @param lpStartAddress Entry point function.
             * @param lpParameter Parameter passed to the entry point.
             * @param dwCreationFlags Flags (e.g., CREATE_SUSPENDED).
             * @param lpThreadId [out] Pointer to receive the new thread ID.
             * @return A Thread object owning the new handle.
             */
            static Thread create(
                LPSECURITY_ATTRIBUTES lpThreadAttributes, 
                DWORD dwStackSize,
                LPTHREAD_START_ROUTINE lpStartAddress,
                LPVOID lpParameter,
                DWORD dwCreationFlags,
                LPDWORD lpThreadId) noexcept;
            /** @} */

        private:
            static void close_handle_(HANDLE h) noexcept;
            void initialize_() noexcept;
            void set_zero_() noexcept;
            static bool is_valid_handle(HANDLE h) noexcept;
    };

    /** @brief Global swap overload for core::General::Thread. */
    extern void swap(Thread& a, Thread& b) noexcept;

} // namespace core::General


#endif // THREAD_H