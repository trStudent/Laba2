/**
 * @file Process.cpp
 * @brief Implementation of the Process RAII wrapper class.
 * @author Timofei Romanchuck
 * @date 2026-01-03
 */

#include <core/General/Process.h>
#include <locale>
#include <codecvt>

static std::wstring utf8_to_wstring(const std::string& str) noexcept {
    if (str.empty()) 
        return std::wstring();

    // 1. Determine required wide-character buffer size
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
        
    if (0 >= size_needed) 
        return std::wstring();

    std::wstring result(size_needed, 0);

    // 2. Map UTF-8 to UTF-16
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size_needed);
        
    // 3. Remove trailing null if MultiByteToWideChar included it
    if (L'\0' == result.back())
        result.pop_back();
        
    return result;
}

namespace core::General {

    Process::Process() noexcept
        : hProcess_(nullptr), hThread_(nullptr), pid_(0), tid_(0)
    { }

    void Process::initialize_() noexcept
    {
        if(nullptr != hProcess_ && nullptr != hThread_)
        {
            // Sync IDs from OS in case they weren't provided in the constructor
            pid_ = GetProcessId(hProcess_);
            if(INVALID_ID == pid_)
            {
                reset();
                return;
            }
            tid_ = GetThreadId(hThread_);
            if(INVALID_ID == tid_)
            {
                reset();
                return;
            }
        } else
            reset();
    }

    void Process::set_zero_() noexcept
    {
        // Nullify state without triggering CloseHandle (used during moves/release)
        hProcess_ = nullptr;
        hThread_ = nullptr;
        pid_ = 0;
        tid_ = 0;
    }

    Process::Process(HANDLE Process_handle,
                    HANDLE thread_handle,
                    DWORD pid,
                    DWORD tid) noexcept
        : hProcess_(Process_handle), hThread_(thread_handle), pid_(pid), tid_(tid)
    {
        this->initialize_();
    }

    Process::Process(const PROCESS_INFORMATION& pi) noexcept
        : hProcess_(pi.hProcess), hThread_(pi.hThread), pid_(pi.dwProcessId), tid_(pi.dwThreadId)
    { 
        this->initialize_();
    }

    Process::Process(Process&& other_) noexcept
    {
        // Transfer ownership and stop the source from closing the handles
        this->hProcess_ = other_.hProcess_;
        this->hThread_ = other_.hThread_;
        this->pid_ = other_.pid_;
        this->tid_ = other_.tid_;
        other_.set_zero_();
    }

    Process& Process::operator=(Process&& other_) noexcept
    {
        if(&other_ != this)
        {
            reset(); // Clean up current resources first
            this->hProcess_ = other_.hProcess_;
            this->hThread_ = other_.hThread_;
            this->pid_ = other_.pid_;
            this->tid_ = other_.tid_;
            other_.set_zero_();
        }
        return *this;
    }

    Process::~Process() noexcept
    {
        reset();
    }

    bool Process::valid() const noexcept
    {
        return is_valid_handle(hProcess_);
    }

    Process::operator bool() const noexcept
    {
        return valid();
    } 

    HANDLE Process::handle() const noexcept
    { return hProcess_; }

    HANDLE Process::thread_handle() const noexcept
    { return hThread_; }

    DWORD Process::pid() const noexcept
    { return pid_; }

    DWORD Process::tid() const noexcept
    { return tid_; }

    std::pair<HANDLE, HANDLE> Process::release() noexcept
    {
        std::pair<HANDLE, HANDLE> result = {hProcess_, hThread_};
        set_zero_(); // Abandon handles to caller
        return result;
    }

    void Process::reset() noexcept
    {
        // Thread handle is closed before process handle (reverse order of allocation)
        close_handle_(hThread_);
        close_handle_(hProcess_);
        set_zero_();
    }

    void Process::reset(HANDLE process_handle,
                    HANDLE thread_handle,
                    DWORD pid,
                    DWORD tid) noexcept
    {
        reset();
        hProcess_ = process_handle;
        hThread_ = thread_handle;
        pid_ = pid;
        tid_ = tid;
        this->initialize_();
    }

    void Process::swap(Process& other_) noexcept
    {
        PROCESS_INFORMATION temp = { hProcess_, hThread_, pid_, tid_};
        this->hThread_ = other_.hThread_;   other_.hThread_ = temp.hThread;
        this->hProcess_ = other_.hProcess_; other_.hProcess_ = temp.hProcess;
        this->pid_ = other_.pid_;           other_.pid_ = temp.dwProcessId;
        this->tid_ = other_.tid_;           other_.tid_ = temp.dwThreadId;
    }

    wait_status Process::wait() noexcept {
        if (valid()) {
            DWORD result = WaitForSingleObject(hProcess_, INFINITE);
            return static_cast<wait_status>(result);
        } else
            return wait_status::failed;
    }

    wait_status Process::wait_for(milliseconds timeout) noexcept {
        if (valid()) {
            DWORD ms = static_cast<DWORD>(timeout.count());
            // Clamp value to avoid collision with INFINITE (0xFFFFFFFF) bitmask
            if(MAX_WAIT_TIMEOUT < ms)
                ms = MAX_WAIT_TIMEOUT;
            DWORD result = WaitForSingleObject(hProcess_, ms);
            return static_cast<wait_status>(result);
        } else 
            return wait_status::failed;
    }

    std::optional<DWORD> Process::try_exit_code() const noexcept
    {
        if(valid()) {
            DWORD exitCode;
            // 259 (STILL_ACTIVE) is a special OS status indicating work in progress
            if(GetExitCodeProcess(hProcess_, &exitCode) && STILL_ACTIVE != exitCode)
                return exitCode;
            else return std::nullopt;
        } else return std::nullopt;
    }

    bool Process::is_running() const noexcept
    {
        return !(try_exit_code().has_value());
    }

    bool Process::terminate(UINT exit_code) noexcept
    {
        if(valid())
            return TerminateProcess(hProcess_, exit_code);
        else return false;
    }

    bool Process::set_priority_class(DWORD priority_class) noexcept
    {
        if(valid())
            return SetPriorityClass(hProcess_, priority_class);
        else return false;
    }

    DWORD Process::get_priority_class() const noexcept
    {
        if(valid())
            return GetPriorityClass(hProcess_);
        else return 0;
    }

    bool Process::suspend() noexcept {
        if (valid()) {
            return THREAD_ERROR_STATUS != SuspendThread(hThread_);
        }
        return false;
    }

    bool Process::resume() noexcept {
        if (valid()) {
            return THREAD_ERROR_STATUS != ResumeThread(hThread_);
        }
        return false;
    }

    Process Process::create(const wchar_t* an,
                                wchar_t* cl,
                                const SECURITY_ATTRIBUTES* pa,
                                const SECURITY_ATTRIBUTES* ta,
                                bool ih,
                                DWORD cf,
                                void* e,
                                const wchar_t* cd,
                                const STARTUPINFOW* si) noexcept
    {
        PROCESS_INFORMATION pi;
        if(CreateProcessW(an, cl, (LPSECURITY_ATTRIBUTES)pa, (LPSECURITY_ATTRIBUTES)ta, ih, cf, e, cd, (LPSTARTUPINFOW)si, &pi))
            return Process(pi);
        else return Process();
    }

    Process Process::create(const std::wstring an,
                                std::wstring cl,
                                const SECURITY_ATTRIBUTES* pa,
                                const SECURITY_ATTRIBUTES* ta,
                                bool ih,
                                DWORD cf,
                                void* e,
                                const std::wstring cd,
                                STARTUPINFOW si) noexcept
    {
        PROCESS_INFORMATION pi;
        // Map empty strings to nullptr as WinAPI expects null for optional paths
        const wchar_t* cs = (cd.empty() ? nullptr : cd.c_str());
        const wchar_t* as = (an.empty() ? nullptr : an.c_str());
        // cl buffer must be writable; passing nullptr if empty
        wchar_t* ps = (cl.empty() ? nullptr : &cl[0]);
        if(CreateProcessW(as, ps, (LPSECURITY_ATTRIBUTES)pa, (LPSECURITY_ATTRIBUTES)ta, ih, cf, e, cs, (LPSTARTUPINFOW)&si, &pi))
            return Process(pi);
        else return Process();
    }

    Process Process::create_utf8(const std::string application_name,
                            std::string command_line,
                            const SECURITY_ATTRIBUTES* pa,
                            const SECURITY_ATTRIBUTES* ta,
                            bool ih,
                            DWORD cf,
                            void* e,
                            const std::string current_directory,
                            STARTUPINFOW si) noexcept
    {
        // Bridge UTF-8 strings to the native wide-character factory
        std::wstring an = utf8_to_wstring(application_name);
        std::wstring cl = utf8_to_wstring(command_line);
        std::wstring cd = utf8_to_wstring(current_directory);

        return create(an, cl, pa, ta, ih, cf, e, cd, si);
    }

    void Process::close_handle_(HANDLE h) noexcept
    {
        if(is_valid_handle(h))
            CloseHandle(h);
    }

    bool Process::is_valid_handle(HANDLE h) noexcept 
    { 
        // WinAPI is inconsistent: some calls return NULL, others return -1 on error
        return nullptr != h && INVALID_HANDLE_VALUE != h; 
    }

    void swap(Process& a, Process& b) noexcept
    {
        a.swap(b);
    }
} // namespace core::General