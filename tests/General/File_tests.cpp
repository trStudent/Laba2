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

    void SetUp() override {
        char tempPath[MAX_PATH] = {};
        DWORD len = GetTempPathA(MAX_PATH, tempPath);
        ASSERT_NE(len, 0);

        char tempFile[MAX_PATH] = {};
        UINT r = GetTempFileNameA(tempPath, "tst", 0, tempFile);
        ASSERT_NE(r, 0);

        temp_dir_ = tempPath;
        temp_file_path_ = tempFile;
    }

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
    EXPECT_FALSE(f.is_opened());
    EXPECT_FALSE(static_cast<bool>(f));
    EXPECT_FALSE(f.close());
}

TEST_F(FileTest, OpenNonExistingForReadFails) {
    std::string path = temp_file_path_;
    DeleteFileA(path.c_str());

    File f = OpenReadExisting(path);
    EXPECT_FALSE(f.is_opened());
    EXPECT_FALSE(static_cast<bool>(f));

    char c;
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

    auto fs0 = f.getFileSize();
    ASSERT_TRUE(fs0.has_value());
    EXPECT_EQ(fs0.value(), 0u);

    EXPECT_TRUE(f.write(data, size));

    auto fs1 = f.getFileSize();
    ASSERT_TRUE(fs1.has_value());
    EXPECT_EQ(fs1.value(), 5u);

    auto fp1 = f.getFilePointer();
    ASSERT_TRUE(fp1.has_value());
    EXPECT_EQ(fp1.value(), 5u);

    ASSERT_TRUE(f.setFilePointer(0));
    auto fp2 = f.getFilePointer();
    ASSERT_TRUE(fp2.has_value());
    EXPECT_EQ(fp2.value(), 0u);

    std::vector<char> buf(size);
    EXPECT_TRUE(f.read(buf.data(), buf.size()));
    EXPECT_EQ(std::string(buf.begin(), buf.end()), data);


    EXPECT_TRUE(f.close());
    EXPECT_FALSE(f.close());
}

TEST_F(FileTest, GetChSequentialRead) {
    File f = OpenRWCreateAlways(temp_file_path_);
    ASSERT_TRUE(f.is_opened());

    const char* msg = "ABC";
    ASSERT_TRUE(f.write(msg, 3));
    ASSERT_TRUE(f.setFilePointer(0));

    auto c1 = f.getCh();
    ASSERT_TRUE(c1.has_value());
    EXPECT_EQ(c1.value(), 'A');

    auto c2 = f.getCh();
    ASSERT_TRUE(c2.has_value());
    EXPECT_EQ(c2.value(), 'B');

    auto c3 = f.getCh();
    ASSERT_TRUE(c3.has_value());
    EXPECT_EQ(c3.value(), 'C');

    auto c4 = f.getCh();
    EXPECT_FALSE(c4.has_value());

    EXPECT_TRUE(f.close());
}

TEST_F(FileTest, ReadPastEndFailsAndEofTrue) {
    File f = OpenRWCreateAlways(temp_file_path_);
    ASSERT_TRUE(f.is_opened());

    const char* msg = "XY";
    ASSERT_TRUE(f.write(msg, 2));

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

    EXPECT_TRUE(f.setFilePointer(4));
    auto fp = f.getFilePointer();
    ASSERT_TRUE(fp.has_value());
    EXPECT_EQ(fp.value(), 4u);

    char c = 0;
    EXPECT_TRUE(f.read(&c, 1));
    EXPECT_EQ(c, '4');

    fp = f.getFilePointer();
    ASSERT_TRUE(fp.has_value());
    EXPECT_EQ(fp.value(), 5u);

    EXPECT_TRUE(f.close());
}

TEST_F(FileTest, MoveSemantics) {
    File f1 = OpenRWCreateAlways(temp_file_path_);
    ASSERT_TRUE(f1.is_opened());

    File f2 = std::move(f1);
    EXPECT_FALSE(f1.is_opened());
    EXPECT_TRUE(f2.is_opened());

    File f3;
    f3 = std::move(f2);
    EXPECT_FALSE(f2.is_opened());
    EXPECT_TRUE(f3.is_opened());

    const char* s = "ok";
    EXPECT_TRUE(f3.write(s, 2));
    EXPECT_TRUE(f3.close());
}