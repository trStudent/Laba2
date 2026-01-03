#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <gtest/gtest.h>
#include <core/General/Process.h>
#include <core/General/Type.h>

using namespace core::General;

class ProcessTest : public ::testing::Test {
protected:
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    void SetUp() override {
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
    }

    void TearDown() override { }
};

TEST_F(ProcessTest, Process_DefaultCtorIsInvalid)
{
    Process p;
    EXPECT_FALSE(p.valid());
    EXPECT_FALSE(static_cast<bool>(p));
    EXPECT_EQ(p.handle(), nullptr);
    EXPECT_EQ(p.thread_handle(), nullptr);
    EXPECT_EQ(p.pid(), 0u);
    EXPECT_EQ(p.tid(), 0u);
}

TEST_F(ProcessTest, MoveSemantics) {
    std::wstring cmd = L"cmd.exe /C exit 0";

    Process p = Process::create(
        L"",
        cmd,
        nullptr,
        nullptr,
        false,
        0,
        nullptr,
        L"",
        si
    );

    ASSERT_TRUE(p.valid());

    Process p2 = std::move(p);
    EXPECT_FALSE(p.valid());
    EXPECT_TRUE(p2.valid());

    Process p3;
    p3 = std::move(p2);
    EXPECT_FALSE(p2.valid());
    EXPECT_TRUE(p3.valid());

    auto [hproc, hthread] = p3.release();
    EXPECT_NE(hproc, nullptr);
    EXPECT_NE(hthread, nullptr);
    EXPECT_FALSE(p3.valid());

    if (hthread) CloseHandle(hthread);
    if (hproc) CloseHandle(hproc);
}

TEST_F(ProcessTest, CodeCheck) {
    std::wstring cmd = L"cmd.exe /C exit 28";

    Process p = Process::create(
        L"",
        cmd,
        nullptr,
        nullptr,
        false,
        0,
        nullptr,
        L"",
        si
    );

    ASSERT_TRUE(p.valid());

    auto ws = p.wait();
    EXPECT_EQ(ws, wait_status::signaled);

    auto code = p.try_exit_code();
    ASSERT_TRUE(code.has_value());

    EXPECT_EQ(code.value(), 28);
    EXPECT_FALSE(p.is_running());
}

TEST_F(ProcessTest, WaitTimeoutWhenProcessStillRunning) {
    std::wstring cmd = L"cmd.exe /C ping 127.0.0.1 -n 3 > NUL";

    Process p = Process::create(
        L"",
        cmd,
        nullptr,
        nullptr,
        false,
        0,
        nullptr,
        L"",
        si
    );

    ASSERT_TRUE(p.valid());

    auto ws = p.wait_for(milliseconds(100));
    EXPECT_EQ(ws, wait_status::timeout);

    EXPECT_TRUE(p.is_running());

    ws = p.wait();
    EXPECT_EQ(ws, wait_status::signaled);
    EXPECT_FALSE(p.is_running());
}

TEST_F(ProcessTest, TerminateRunningProcess) {
    std::wstring cmd = L"cmd.exe /C ping 127.0.0.1 -n 6 > NUL";

    Process p = Process::create(
        L"",
        cmd,
        nullptr,
        nullptr,
        false,
        0,
        nullptr,
        L"",
        si
    );

    ASSERT_TRUE(p.valid());

    EXPECT_TRUE(p.is_running());
    EXPECT_TRUE(p.terminate(42));

    auto ws = p.wait();
    EXPECT_EQ(ws, wait_status::signaled);

    auto ec = p.try_exit_code();
    ASSERT_TRUE(ec.has_value());
    EXPECT_EQ(ec.value(), 42u);
}

TEST_F(ProcessTest, SuspendAndResumeThread) {
    std::wstring cmd = L"cmd.exe /C exit 0";

    Process p = Process::create(
        L"",
        cmd,
        nullptr,
        nullptr,
        false,
        0,
        nullptr,
        L"",
        si
    );

    ASSERT_TRUE(p.valid());

    EXPECT_TRUE(p.resume());

    EXPECT_TRUE(p.suspend());
    EXPECT_TRUE(p.resume());

    EXPECT_EQ(p.wait(), wait_status::signaled);
    EXPECT_FALSE(p.is_running());
}

TEST_F(ProcessTest, ResetAndSwap) {
    std::wstring cmd1 = L"cmd.exe /C exit 0";
    std::wstring cmd2 = L"cmd.exe /C exit 3";

    Process a = Process::create(L"", cmd1, nullptr, nullptr, false, 0, nullptr, L"", si);
    Process b = Process::create(L"", cmd2, nullptr, nullptr, false, 0, nullptr, L"", si);

    ASSERT_TRUE(a.valid());
    ASSERT_TRUE(b.valid());

    DWORD a_pid = a.pid();
    DWORD b_pid = b.pid();

    a.swap(b);
    EXPECT_EQ(a.pid(), b_pid);
    EXPECT_EQ(b.pid(), a_pid);

    swap(a, b);
    EXPECT_EQ(a.pid(), a_pid);
    EXPECT_EQ(b.pid(), b_pid);

    b.reset();
    EXPECT_FALSE(b.valid());

    EXPECT_TRUE(a.valid());
    a.wait();
}

TEST_F(ProcessTest, CreateUtf8Overload) {
    std::string cmd = "cmd.exe /C exit 28";

    Process p = Process::create_utf8(
        "",
        cmd,
        nullptr,
        nullptr,
        false,
        0,
        nullptr,
        "",
        si
    );

    ASSERT_TRUE(p.valid());

    auto ws = p.wait();
    EXPECT_EQ(ws, wait_status::signaled);

    auto code = p.try_exit_code();
    ASSERT_TRUE(code.has_value());

    EXPECT_EQ(code.value(), 28);
    EXPECT_FALSE(p.is_running());
}

TEST_F(ProcessTest, InvalidHandlesBehaveAsInvalid) {
    Process p(nullptr, nullptr, 0, 0);
    EXPECT_FALSE(p.valid());
    EXPECT_EQ(p.wait_for(milliseconds(1)), wait_status::failed);
    EXPECT_FALSE(p.terminate());
    EXPECT_FALSE(p.resume());
    EXPECT_FALSE(p.suspend());
}