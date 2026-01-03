/**
 * @file File.cpp
 * @brief Implementation of the File RAII wrapper for Windows file handles.
 * @author Timofei Romanchuck
 * @date 2026-01-03
 */

#include <core/General/File.h>

namespace core::General
{
    void File::set_zero_() noexcept
    {
        // Standard Win32 invalid handle state
        hFile_ = INVALID_HANDLE_VALUE;
    }

    File::File(HANDLE h) noexcept
        : hFile_(h)
    { }

    File::File() noexcept
        : hFile_(INVALID_HANDLE_VALUE)
    { }

    File::File(File&& other) noexcept
        : hFile_(other.hFile_)
    { 
        // Move semantics: transfer ownership and invalidate source
        other.set_zero_(); 
    }

    File& File::operator=(File&& other) noexcept
    {
        if(&other != this)
        {
            // Ensure existing resource is closed before taking a new one
            close();
            hFile_ = other.hFile_;
            other.set_zero_();
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
        // Win32 handles can be invalid as either NULL or -1 depending on creation context
        return INVALID_HANDLE_VALUE != hFile_ && nullptr != hFile_;
    }

    bool File::write(const char* buf, DWORD size) const noexcept
    {
        DWORD dwBytesWritten = 0;
        BOOL writeFile = WriteFile(hFile_, buf, size, &dwBytesWritten, nullptr);
        // Success requires both the API call to succeed and the full buffer to be flushed
        return (writeFile && dwBytesWritten > 0);
    }

    bool File::read(char* buf, DWORD size) const noexcept
    { 
        if(0 == size) return true;
        
        DWORD dwBytesRead = 0;
        BOOL readFile = ReadFile(hFile_, buf, size, &dwBytesRead, nullptr);
        // We consider a read successful only if data was actually moved into the buffer
        return (readFile && dwBytesRead > 0);
    }

    void File::ignore(char delim, size_t s) const noexcept
    {
        std::optional<char> a;
        if(s)
            // Sequentially consume characters until limit, delimiter, or EOF
            while((a = getCh()).has_value() && a.value() != delim && (--s));
    }

    std::optional<char> File::getCh() const noexcept
    {
        char ch;
        const DWORD ONE_BYTE = 1;
        if(read(&ch, ONE_BYTE))
            return ch;
        
        return std::nullopt;
    }

    bool File::close() noexcept
    {
        if(is_opened())
        {
            BOOL res = CloseHandle(hFile_);
            set_zero_();
            return res;
        } 
        
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
        // Wrapper for the native Win32 file creation/opening function
        HANDLE hFile_ = CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        return File(hFile_);
    }

    std::optional<DWORD> File::getFilePointer() const noexcept
    {
        if(is_opened()) {
            // Passing 0 and FILE_CURRENT allows us to query the position without moving it
            DWORD dwLow = SetFilePointer(hFile_, 0, nullptr, FILE_CURRENT);
            if(INVALID_SET_FILE_POINTER == dwLow)
                return std::nullopt;
            return dwLow;
        } 
        
        return std::nullopt;
    }

    bool File::setFilePointer(DWORD p) const noexcept
    {
        if(is_opened()) {
            // Moves the file pointer to an absolute position relative to the beginning
            DWORD dwLow = SetFilePointer(hFile_, p, nullptr, FILE_BEGIN);
            return INVALID_SET_FILE_POINTER != dwLow;
        } 
        
        return false;
    }

    std::optional<DWORD> File::getFileSize() const noexcept
    {
        if(is_opened()) {
            DWORD dwLow = GetFileSize(hFile_, nullptr);
            if(INVALID_FILE_SIZE == dwLow)
                return std::nullopt;
            return dwLow;
        } 
        
        return std::nullopt;
    }
} // core::General