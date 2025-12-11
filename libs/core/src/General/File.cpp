#include <core/General/File.h>

namespace core::General
{

    void File::set_zero_() noexcept
    {
        hFile_ = INVALID_HANDLE_VALUE;
    }

    File::File(HANDLE h) noexcept
        : hFile_(h)
    { }

    File::File() noexcept
        : hFile_(INVALID_HANDLE_VALUE)
    { }

    File::File(File&& other_) noexcept
        : hFile_(other_.hFile_)
    { other_.set_zero_(); }

    File& File::operator=(File&& other_) noexcept
    {
        if(this != &other_)
        {
            close();
            hFile_ = other_.hFile_;
            other_.set_zero_();
        }
        return *this;
    }

    File::~File() noexcept
    {
        close();
        set_zero_();
    }

    File::operator bool() const noexcept
    {
        return is_opened();
    }

    bool File::is_opened() const noexcept
    {
        return hFile_ != INVALID_HANDLE_VALUE && hFile_ != nullptr;
    }

    bool File::write(const char* buf, DWORD size) const noexcept
    {
        DWORD dwBytesWritten = 0;
        BOOL writeFile = WriteFile(hFile_, buf, size, &dwBytesWritten, nullptr);
        return (writeFile && dwBytesWritten > 0);
    }

    bool File::read(char* buf, DWORD size) const noexcept
    { 
        DWORD dwBytesRead = 0;
        BOOL readFile = ReadFile(hFile_, buf, size, &dwBytesRead, nullptr);
        return (readFile && dwBytesRead > 0);
    }

    void File::ignore(char delim, size_t s) const noexcept
    {
        std::optional<char> a;
        if(s)
            while((a = getCh()).has_value() && a.value() != delim && (--s));
    }

    std::optional<char> File::getCh() const noexcept
    {
        char ch;
        if(read(&ch, 1))
            return ch;
        else return std::nullopt;
    }

    bool File::close() noexcept
    {
        if(is_opened())
        {
            BOOL res = CloseHandle(hFile_);
            set_zero_();
            return res;
        } else
            return false;
    }

    File File::open(LPCSTR lpFileName,
                    DWORD dwDesiredAccess,
                    DWORD dwShareMode,
                    LPSECURITY_ATTRIBUTES  lpSecurityAttributes,
                    DWORD dwCreationDisposition,
                    DWORD dwFlagsAndAttributes,
                    HANDLE hTemplateFile)
    {
        HANDLE hFile_ = CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        return File(hFile_);
    }

    std::optional<DWORD> File::getFilePointer() const noexcept
    {
        if(is_opened()) {
            LONG lHigh = 0;
            DWORD dwLow = SetFilePointer(hFile_, NULL, nullptr, FILE_CURRENT);
            if(dwLow == INVALID_SET_FILE_POINTER)
                return std::nullopt;
            return dwLow;
        } else return std::nullopt;

    }
    bool File::setFilePointer(DWORD p) const noexcept
    {
        if(is_opened()) {
            DWORD dwLow = SetFilePointer(hFile_, p, nullptr, FILE_BEGIN);
            return dwLow != INVALID_SET_FILE_POINTER;
        } else return false;
    }
    std::optional<DWORD> File::getFileSize() const noexcept
    {
        if(is_opened()) {
            DWORD dwLow = GetFileSize(hFile_, nullptr);
            if(dwLow == INVALID_FILE_SIZE)
                return std::nullopt;
            return dwLow;
        } else return std::nullopt;
    }
} // core::General