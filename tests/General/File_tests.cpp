/**
 * @file File_tests.cpp
 * @brief Unit tests for the File RAII wrapper using GoogleTest.
 * @author Timofei Romanchuck
 * @date 2026-01-03
 */

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <gtest/gtest.h>
#include <Windows.h>
#include <string>
#include <vector>
#include <optional>
#include <cstdio>

#include <core/General/File.h>

using namespace core::General;

class FileTest : public ::testing::Test {
protected:
    std::string temp_dir_;
    std::string temp_file_path_;

    /**
     * SetUp generates a unique temporary file path before each test.
     */
    void SetUp() override {
        char tempPath[MAX_PATH] = {};
        DWORD len = GetTempPathA(MAX_PATH, tempPath);
        ASSERT_NE(0, len);

        char tempFile[MAX_PATH] = {};
        UINT r = GetTempFileNameA(tempPath, "tst", 0, tempFile);
        ASSERT_NE(0, r);

        temp_dir_ = tempPath;
        temp_file_path_ = tempFile;
    }

    /**
     * TearDown ensures disk cleanup by deleting the temporary file.
     */
    void TearDown() override {
        if (!temp_file_path_.empty()) {
            DeleteFileA(temp_file_path_.c_str());
        }
    }

    static File OpenRWCreateAlways(const std::string& path) {
        return File::open(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
    }

    static File OpenReadExisting(const std::string& path) {
        return File::open(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
    }
};

TEST_F(FileTest, DefaultCtorIsInvalid) {
    File f;
    // An uninitialized file should report as closed and return false on conversion
    EXPECT_FALSE(f.is_opened());
    EXPECT_FALSE(static_cast<bool>(f));
    EXPECT_FALSE(f.close());
}

TEST_F(FileTest, OpenNonExistingForReadFails) {
    std::string path = temp_file_path_;
    DeleteFileA(path.c_str());

    File f = OpenReadExisting(path);
    // Opening a missing file should result in an invalid object
    EXPECT_FALSE(f.is_opened());
    EXPECT_FALSE(static_cast<bool>(f));

    char c;
    // All I/O operations must fail on invalid handles
    EXPECT_FALSE(f.read(&c, 1));
    EXPECT_FALSE(f.write("x", 1));

    auto fp = f.getFilePointer();
    EXPECT_FALSE(fp.has_value());

    auto fs = f.getFileSize();
    EXPECT_FALSE(fs.has_value());

    EXPECT_FALSE(f.setFilePointer(0));
    EXPECT_FALSE(f.close());
}

TEST_F(FileTest, CreateWriteSizeAndPointer) {
    File f = OpenRWCreateAlways(temp_file_path_);
    ASSERT_TRUE(f.is_opened());
    ASSERT_TRUE(static_cast<bool>(f));

    const char* data = "hello";
    size_t size = 5;

    // Newly created file must have zero size
    auto fs0 = f.getFileSize();
    ASSERT_TRUE(fs0.has_value());
    EXPECT_EQ(0u, fs0.value());

    EXPECT_TRUE(f.write(data, size));

    // After write, size and pointer should match the amount of data written
    auto fs1 = f.getFileSize();
    ASSERT_TRUE(fs1.has_value());
    EXPECT_EQ(5u, fs1.value());

    auto fp1 = f.getFilePointer();
    ASSERT_TRUE(fp1.has_value());
    EXPECT_EQ(5u, fp1.value());

    // Manual seek back to the beginning to verify read logic
    ASSERT_TRUE(f.setFilePointer(0));
    auto fp2 = f.getFilePointer();
    ASSERT_TRUE(fp2.has_value());
    EXPECT_EQ(0u, fp2.value());

    std::vector<char> buf(size);
    EXPECT_TRUE(f.read(buf.data(), buf.size()));
    EXPECT_EQ(std::string(buf.begin(), buf.end()), data);

    EXPECT_TRUE(f.close());
    EXPECT_FALSE(f.close()); // Double close should fail
}

TEST_F(FileTest, GetChSequentialRead) {
    File f = OpenRWCreateAlways(temp_file_path_);
    ASSERT_TRUE(f.is_opened());

    const char* msg = "ABC";
    ASSERT_TRUE(f.write(msg, 3));
    ASSERT_TRUE(f.setFilePointer(0));

    // Verify character-by-character extraction moves the pointer correctly
    auto c1 = f.getCh();
    ASSERT_TRUE(c1.has_value());
    EXPECT_EQ('A', c1.value());

    auto c2 = f.getCh();
    ASSERT_TRUE(c2.has_value());
    EXPECT_EQ('B', c2.value());

    auto c3 = f.getCh();
    ASSERT_TRUE(c3.has_value());
    EXPECT_EQ('C', c3.value());

    // EOF check
    auto c4 = f.getCh();
    EXPECT_FALSE(c4.has_value());

    EXPECT_TRUE(f.close());
}

TEST_F(FileTest, ReadPastEndFailsAndEofTrue) {
    File f = OpenRWCreateAlways(temp_file_path_);
    ASSERT_TRUE(f.is_opened());

    const char* msg = "XY";
    ASSERT_TRUE(f.write(msg, 2));

    // Attempting to read when the pointer is at EOF should fail
    ASSERT_TRUE(f.setFilePointer(2));
    char b = 0;
    EXPECT_FALSE(f.read(&b, 1));

    EXPECT_TRUE(f.close());
}

TEST_F(FileTest, SetAndGetFilePointerArbitrary) {
    File f = OpenRWCreateAlways(temp_file_path_);
    ASSERT_TRUE(f.is_opened());

    const char* payload = "0123456789";
    ASSERT_TRUE(f.write(payload, 10));

    // Move to the middle of the file
    EXPECT_TRUE(f.setFilePointer(4));
    auto fp = f.getFilePointer();
    ASSERT_TRUE(fp.has_value());
    EXPECT_EQ(4u, fp.value());

    char c = 0;
    EXPECT_TRUE(f.read(&c, 1));
    EXPECT_EQ('4', c);

    // Verify pointer incremented after single-byte read
    fp = f.getFilePointer();
    ASSERT_TRUE(fp.has_value());
    EXPECT_EQ(5u, fp.value());

    EXPECT_TRUE(f.close());
}

TEST_F(FileTest, MoveSemantics) {
    File f1 = OpenRWCreateAlways(temp_file_path_);
    ASSERT_TRUE(f1.is_opened());

    // Ownership transfer: f2 takes the handle, f1 becomes invalid
    File f2 = std::move(f1);
    EXPECT_FALSE(f1.is_opened());
    EXPECT_TRUE(f2.is_opened());

    // Assignment transfer
    File f3;
    f3 = std::move(f2);
    EXPECT_FALSE(f2.is_opened());
    EXPECT_TRUE(f3.is_opened());

    const char* s = "ok";
    EXPECT_TRUE(f3.write(s, 2));
    EXPECT_TRUE(f3.close());
}