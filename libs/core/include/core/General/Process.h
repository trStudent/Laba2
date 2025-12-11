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
#include "Thread.h"

namespace core::General
{
    class Process {
    private:
        HANDLE hProcess_;
        HANDLE hThread_;
        DWORD pid_;
        DWORD tid_;
    public:
        Process() noexcept;
        Process(HANDLE Process_handle,
                HANDLE thread_handle,
                DWORD pid = 0,
                DWORD tid = 0) noexcept;
        explicit Process(const PROCESS_INFORMATION& pi) noexcept;

        Process(const Process&) = delete;
        Process& operator=(const Process&) = delete;

        Process(Process&& other_) noexcept;
        Process& operator=(Process&& other_) noexcept;

        ~Process() noexcept;

        bool valid() const noexcept;
        operator bool() const noexcept;

        HANDLE handle() const noexcept;
        HANDLE thread_handle() const noexcept;
        DWORD pid() const noexcept;
        DWORD tid() const noexcept;

        std::pair<HANDLE, HANDLE> release() noexcept;
        void reset() noexcept;
        void reset(HANDLE process_handle,
                HANDLE thread_handle,
                DWORD pid = 0,
                DWORD tid = 0) noexcept;

        void swap(Process& other_) noexcept;

        wait_status wait() noexcept;
        wait_status wait_for(milliseconds timeout) noexcept;

        std::optional<DWORD> try_exit_code() const noexcept;
        bool is_running() const noexcept;

        bool terminate(UINT exit_code = 0) noexcept;

        bool set_priority_class(DWORD priority_class) noexcept;
        DWORD get_priority_class() const noexcept;

        bool suspend() noexcept;
        bool resume() noexcept;

        static Process create(const wchar_t* application_name,
                            wchar_t* command_line,
                            const SECURITY_ATTRIBUTES* process_attrs,
                            const SECURITY_ATTRIBUTES* thread_attrs,
                            bool inherit_handles,
                            DWORD creation_flags,
                            void* environment,
                            const wchar_t* current_directory,
                            const STARTUPINFOW* startup_info) noexcept;

        static Process create(const std::wstring application_name,
                            std::wstring command_line,
                            const SECURITY_ATTRIBUTES* process_attrs,
                            const SECURITY_ATTRIBUTES* thread_attrs,
                            bool inherit_handles,
                            DWORD creation_flags,
                            void* environment,
                            const std::wstring current_directory,
                            STARTUPINFOW startup_info) noexcept;

        static Process create_utf8(const std::string application_name,
                                std::string command_line,
                                const SECURITY_ATTRIBUTES* process_attrs,
                                const SECURITY_ATTRIBUTES* thread_attrs,
                                bool inherit_handles,
                                DWORD creation_flags,
                                void* environment,
                                const std::string current_directory,
                                STARTUPINFOW startup_info) noexcept;
    private:
        static void close_handle_(HANDLE h) noexcept;
        void initialize_() noexcept;
        void set_zero_() noexcept;
        static bool is_valid_handle(HANDLE h) noexcept;
    };

    extern void swap(Process& a, Process& b) noexcept;
} // namespace core::General

#endif // PROCESS_H