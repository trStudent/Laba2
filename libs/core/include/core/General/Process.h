/**
 * @file Process.h
 * @brief RAII wrapper for Windows Process and Primary Thread handles.
 * @author Timofei Romanchuck
 * @date 2026-01-03
 */

#ifndef PROCESS_H
#define PROCESS_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <chrono>
#include <optional>
#include <string>
#include <utility>
#include "Type.h"

/**
 * @namespace core::General
 * @brief Core utilities for system object management.
 */
namespace core::General
{
    /**
     * @class Process
     * @brief Manages the lifecycle of a Windows process and its primary thread.
     * 
     * Provides an RAII wrapper that ensures both the process and thread handles
     * are closed automatically. Ownership can be transferred via move semantics
     * but not copied.
     */
    class Process {
    private:
        HANDLE hProcess_; /**< Internal handle to the process. */
        HANDLE hThread_;  /**< Internal handle to the primary thread. */
        DWORD pid_;       /**< Cached Process ID. */
        DWORD tid_;       /**< Cached Thread ID. */

        /** @name Internal Constants
         *  @{ */
        static constexpr DWORD INVALID_ID = 0;
        static constexpr DWORD THREAD_ERROR_STATUS = static_cast<DWORD>(-1);
        static constexpr DWORD MAX_WAIT_TIMEOUT = INFINITE - 1;
        static constexpr DWORD FAILED_PRIORITY = 0;
        /** @} */

    public:
        /** @name Constructors and Destructor
         *  @{ */

        /** @brief Constructs an empty Process object with null handles. */
        Process() noexcept;

        /**
         * @brief Wraps existing handles. 
         * Calls initialize_() to verify and cache IDs.
         */
        Process(HANDLE Process_handle,
                HANDLE thread_handle,
                DWORD pid = 0,
                DWORD tid = 0) noexcept;

        /** @brief Constructs from PROCESS_INFORMATION structure returned by WinAPI. */
        explicit Process(const PROCESS_INFORMATION& pi) noexcept;

        /** @brief Copying is prohibited to prevent double-closing of handles. */
        Process(const Process&) = delete;
        /** @brief Copying is prohibited to prevent double-closing of handles. */
        Process& operator=(const Process&) = delete;

        /** @brief Move constructor. Transfers ownership of handles. */
        Process(Process&& other_) noexcept;
        /** @brief Move assignment. Closes current handles before taking ownership. */
        Process& operator=(Process&& other_) noexcept;

        /** @brief Destructor. Automatically calls reset() to close handles. */
        ~Process() noexcept;
        /** @} */

        /** @name Utility and Status
         *  @{ */

        /** @return true if the process handle is not NULL and not INVALID_HANDLE_VALUE. */
        bool valid() const noexcept;
        
        /** @brief Explicit check for validity. */
        operator bool() const noexcept;

        /** @return The raw Win32 process handle. */
        HANDLE handle() const noexcept;
        /** @return The raw Win32 primary thread handle. */
        HANDLE thread_handle() const noexcept;
        /** @return The cached process ID. */
        DWORD pid() const noexcept;
        /** @return The cached thread ID. */
        DWORD tid() const noexcept;

        /**
         * @brief Releases ownership of handles.
         * @return A pair of {ProcessHandle, ThreadHandle}. The object becomes invalid.
         */
        std::pair<HANDLE, HANDLE> release() noexcept;

        /** @brief Closes handles and clears all internal IDs. */
        void reset() noexcept;

        /** @brief Closes current handles and takes ownership of new ones. */
        void reset(HANDLE process_handle,
                HANDLE thread_handle,
                DWORD pid = 0,
                DWORD tid = 0) noexcept;

        /** @brief Exchanges the state of this object with another. */
        void swap(Process& other_) noexcept;
        /** @} */

        /** @name Synchronization and Execution
         *  @{ */

        /**
         * @brief Blocks indefinitely until the process terminates.
         * @return wait_status::signaled on termination, or wait_status::failed.
         */
        wait_status wait() noexcept;

        /**
         * @brief Blocks for a limited time until process terminates.
         * @param timeout The duration to wait.
         * @return signaled if terminated, timeout if time ran out, failed if invalid.
         */
        wait_status wait_for(milliseconds timeout) noexcept;

        /**
         * @brief Retrieves the exit code of the process.
         * @return The exit code, or std::nullopt if process is still running or handle is invalid.
         */
        std::optional<DWORD> try_exit_code() const noexcept;

        /**
         * @brief Checks if the process is currently active.
         * @return true if try_exit_code() has no value (running or invalid).
         */
        bool is_running() const noexcept;

        /** @brief Forcibly kills the process with the specified exit code. */
        bool terminate(UINT exit_code = 0) noexcept;

        /** @brief Adjusts the process priority class. */
        bool set_priority_class(DWORD priority_class) noexcept;
        /** @return The priority class or 0 if the call failed. */
        DWORD get_priority_class() const noexcept;

        /** @brief Suspends the primary thread. */
        bool suspend() noexcept;
        /** @brief Resumes the primary thread. */
        bool resume() noexcept;
        /** @} */

        /** @name Static Creation Methods
         *  @{ */

        /** @brief Spawns a process using native Win32 wide strings. */
        static Process create(const wchar_t* application_name,
                            wchar_t* command_line,
                            const SECURITY_ATTRIBUTES* process_attrs,
                            const SECURITY_ATTRIBUTES* thread_attrs,
                            bool inherit_handles,
                            DWORD creation_flags,
                            void* environment,
                            const wchar_t* current_directory,
                            const STARTUPINFOW* startup_info) noexcept;

        /** @brief Spawns a process using std::wstring. Handles nullptr conversions. */
        static Process create(const std::wstring application_name,
                            std::wstring command_line,
                            const SECURITY_ATTRIBUTES* process_attrs,
                            const SECURITY_ATTRIBUTES* thread_attrs,
                            bool inherit_handles,
                            DWORD creation_flags,
                            void* environment,
                            const std::wstring current_directory,
                            STARTUPINFOW startup_info) noexcept;

        /** @brief Spawns a process from UTF-8 strings. Performs internal conversion to wide. */
        static Process create_utf8(const std::string application_name,
                                std::string command_line,
                                const SECURITY_ATTRIBUTES* process_attrs,
                                const SECURITY_ATTRIBUTES* thread_attrs,
                                bool inherit_handles,
                                DWORD creation_flags,
                                void* environment,
                                const std::string current_directory,
                                STARTUPINFOW startup_info) noexcept;
        /** @} */

    private:
        static void close_handle_(HANDLE h) noexcept;
        void initialize_() noexcept;
        void set_zero_() noexcept;
        static bool is_valid_handle(HANDLE h) noexcept;
    };

    /** @brief Global swap overload. */
    extern void swap(Process& a, Process& b) noexcept;
} // namespace core::General

#endif // PROCESS_H