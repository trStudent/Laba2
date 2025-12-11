#include <core/General/Process.h>
#include <locale>
#include <codecvt>

static std::wstring utf8_to_wstring(const std::string& str) noexcept {
    if (str.empty()) 
        return std::wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
        
    if (size_needed <= 0) 
        return std::wstring();

    std::wstring result(size_needed, 0);

    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size_needed);
        
    if (result.back() == L'\0')
        result.pop_back();
        
    return result;
}


namespace core::General {

    Process::Process() noexcept
        : hProcess_(nullptr), hThread_(nullptr), pid_(0), tid_(0)
    { }

    void Process::initialize_() noexcept
    {
        if(hProcess_ != nullptr && hThread_ != nullptr)
        {
            pid_ = GetProcessId(hProcess_);
            if(pid_ == 0)
            {
                reset();
                return;
            }
            tid_ = GetThreadId(hThread_);
            if(tid_ == 0)
            {
                reset();
                return;
            }
        } else
            reset();
    }

    void Process::set_zero_() noexcept
    {
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
        this->hProcess_ = other_.hProcess_;
        this->hThread_ = other_.hThread_;
        this->pid_ = other_.pid_;
        this->tid_ = other_.tid_;
        other_.set_zero_();
    }

    Process& Process::operator=(Process&& other_) noexcept
    {
        if(this != &other_)
        {
            reset();
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
        set_zero_();
        return result;
    }

    void Process::reset() noexcept
    {
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
            if(ms > INFINITE - 1)
                ms = INFINITE - 1;
            DWORD result = WaitForSingleObject(hProcess_, ms);
            return static_cast<wait_status>(result);
        } else 
            return wait_status::failed;
    }

    std::optional<DWORD> Process::try_exit_code() const noexcept
    {
        if(valid()) {
            DWORD exitCode;
            if(GetExitCodeProcess(hProcess_, &exitCode) && exitCode != STILL_ACTIVE)
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

    bool Process::suspend() noexcept
    {
        if(valid())
            return SuspendThread(hThread_) != (DWORD)-1;
        else return false;
    }

    bool Process::resume() noexcept
    {
        if(valid())
            return ResumeThread(hThread_) != (DWORD)-1;
        else return false;
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
        const wchar_t* cs = (cd.empty() ? nullptr : cd.c_str());
        const wchar_t* as = (an.empty() ? nullptr : an.c_str());
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
        return h != nullptr && h != INVALID_HANDLE_VALUE; 
    }

    void swap(Process& a, Process& b) noexcept
    {
        a.swap(b);
    }
} // namespace core::General