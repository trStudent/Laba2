/**
 * @file File.h
 * @brief RAII wrapper for Windows file handles.
 * @author Timofei Romanchuck
 * @date 2026-01-03
 */

#ifndef FILE_H
#define FILE_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <string>
#include <optional>

/**
 * @namespace core::General
 * @brief Namespace for general-purpose system utilities.
 */
namespace core::General
{
    /**
     * @class File
     * @brief A move-only RAII wrapper for Win32 file handles.
     * 
     * This class ensures that file handles are closed automatically when the 
     * object goes out of scope. It provides basic synchronous I/O, file 
     * positioning, and size retrieval.
     */
    class File
    {
    private:
        HANDLE hFile_; /**< The internal Win32 file handle. */

    public:
        /** @name Lifecycle Management
         *  @{ */

        /** @brief Default constructor. Initializes an invalid/closed file object. */
        File() noexcept;

        /** @brief Deleted copy constructor to prevent handle duplication. */
        File(const File& f) = delete;

        /** @brief Move constructor. Transfers handle ownership from @p other. */
        File(File&& other) noexcept;

        /** @brief Wrap an existing Win32 handle. */
        File(HANDLE h) noexcept;

        /** @brief Deleted copy assignment. */
        File& operator=(const File& other) = delete;

        /** @brief Move assignment. Closes current handle and takes ownership from @p other. */
        File& operator=(File&& other) noexcept;

        /** @brief Destructor. Automatically closes the file handle. */
        ~File() noexcept;
        /** @} */

        /** @name Status and Basic I/O
         *  @{ */

        /** @brief Implicit check for file openness. Same as is_opened(). */
        operator bool() const noexcept;

        /** @return true if the file handle is valid and opened. */
        bool is_opened() const noexcept;

        /**
         * @brief Writes data to the file.
         * @param buf Source buffer.
         * @param size Number of bytes to write.
         * @return true if the write was successful.
         */
        bool write(const char* buf, DWORD size) const noexcept;

        /**
         * @brief Reads data from the file.
         * @param buf Destination buffer.
         * @param size Number of bytes to read.
         * @return true if data was successfully read.
         */
        bool read(char* buf, DWORD size) const noexcept;

        /**
         * @brief Skips characters until a delimiter or count limit is reached.
         * @param delim The character to stop at.
         * @param s Maximum number of characters to skip.
         */
        void ignore(char delim, size_t s) const noexcept;

        /**
         * @brief Reads a single character from the file.
         * @return The character read, or std::nullopt on EOF or error.
         */
        std::optional<char> getCh() const noexcept;

        /** @brief Manually closes the file handle. */
        bool close() noexcept;

        /**
         * @brief Static factory to open/create a file via CreateFileA.
         * @return A File object owning the resulting handle.
         */
        static File open(LPCSTR lpFileName,
                          DWORD dwDesiredAccess,
                          DWORD dwShareMode,
                          LPSECURITY_ATTRIBUTES  lpSecurityAttributes,
                          DWORD dwCreationDisposition,
                          DWORD dwFlagsAndAttributes,
                          HANDLE hTemplateFile);
        /** @} */

        /** @name Positioning and Size
         *  @{ */

        /**
         * @brief Retrieves the current file pointer position.
         * @return The byte offset from the start of the file.
         */
        std::optional<DWORD> getFilePointer() const noexcept;

        /**
         * @brief Sets the file pointer to an absolute position.
         * @param p Byte offset from the start of the file.
         * @return true if the pointer was successfully moved.
         */
        bool setFilePointer(DWORD p) const noexcept;

        /** @return The total size of the file in bytes. */
        std::optional<DWORD> getFileSize() const noexcept;
        /** @} */

    private:
        /** @brief Internal helper to nullify the handle. */
        void set_zero_() noexcept;
    };

} // core::General

#endif // FILE_H