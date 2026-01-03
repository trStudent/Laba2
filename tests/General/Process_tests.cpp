/**
 * @file Process_tests.cpp
 * @brief Unit tests for the Process RAII wrapper using GoogleTest.
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
#include <core/General/Process.h>
#include <core/General/Type.h>

using namespace core::General;

class ProcessTest : public ::testing::Test {
protected:
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    /**
     * Initializes the standard Windows startup info structure.
     */
    void SetUp() override {
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
    }

    void TearDown() override { }
};

TEST_F(ProcessTest, Process_DefaultCtorIsInvalid)
{
    Process p;
    // A default-constructed process must hold null handles and zero IDs
    EXPECT_FALSE(p.valid());
    EXPECT_FALSE(static_cast<bool>(p));
    EXPECT_EQ(nullptr, p.handle());
    EXPECT_EQ(nullptr, p.thread_handle());
    EXPECT_EQ(0u, p.pid());
    EXPECT_EQ(0u, p.tid());
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

    // Transfer ownership to p2; p must become invalid
    Process p2 = std::move(p);
    EXPECT_FALSE(p.valid());
    EXPECT_TRUE(p2.valid());

    // Transfer ownership via assignment
    Process p3;
    p3 = std::move(p2);
    EXPECT_FALSE(p2.valid());
    EXPECT_TRUE(p3.valid());

    // Release allows manual handle management; object loses ownership
    auto [hproc, hthread] = p3.release();
    EXPECT_NE(nullptr, hproc);
    EXPECT_NE(nullptr, hthread);
    EXPECT_FALSE(p3.valid());

    // Manual cleanup for released handles
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

    // Blocking wait until cmd.exe finishes
    auto ws = p.wait();
    EXPECT_EQ(wait_status::signaled, ws);

    // Verify the exit code returned by the shell
    auto code = p.try_exit_code();
    ASSERT_TRUE(code.has_value());

    EXPECT_EQ(28, code.value());
    EXPECT_FALSE(p.is_running());
}

TEST_F(ProcessTest, WaitTimeoutWhenProcessStillRunning) {
    // Use ping to simulate a process that runs for approximately 2 seconds
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

    // 100ms wait should result in a timeout status
    auto ws = p.wait_for(milliseconds(100));
    EXPECT_EQ(wait_status::timeout, ws);

    EXPECT_TRUE(p.is_running());

    // Cleanup by waiting for the process to finish normally
    ws = p.wait();
    EXPECT_EQ(wait_status::signaled, ws);
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
    // Forcibly kill process and verify the specific exit code (42)
    EXPECT_TRUE(p.terminate(42));

    auto ws = p.wait();
    EXPECT_EQ(wait_status::signaled, ws);

    auto ec = p.try_exit_code();
    ASSERT_TRUE(ec.has_value());
    EXPECT_EQ(42u, ec.value());
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

    // Verify that primary thread control (suspend/resume) does not crash
    EXPECT_TRUE(p.resume());
    EXPECT_TRUE(p.suspend());
    EXPECT_TRUE(p.resume());

    EXPECT_EQ(wait_status::signaled, p.wait());
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

    // Exchange internal process/thread handles between objects
    a.swap(b);
    EXPECT_EQ(b_pid, a.pid());
    EXPECT_EQ(a_pid, b.pid());

    swap(a, b);
    EXPECT_EQ(a_pid, a.pid());
    EXPECT_EQ(b_pid, b.pid());

    // reset() must close handles and invalidate the object
    b.reset();
    EXPECT_FALSE(b.valid());

    EXPECT_TRUE(a.valid());
    a.wait();
}

TEST_F(ProcessTest, CreateUtf8Overload) {
    std::string cmd = "cmd.exe /C exit 28";

    // Verify string conversion bridge from UTF-8 to Wide characters
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
    EXPECT_EQ(wait_status::signaled, ws);

    auto code = p.try_exit_code();
    ASSERT_TRUE(code.has_value());
    EXPECT_EQ(28, code.value());
}

TEST_F(ProcessTest, InvalidHandlesBehaveAsInvalid) {
    // Manually construct an object with null handles to check safety guards
    Process p(nullptr, nullptr, 0, 0);
    EXPECT_FALSE(p.valid());
    EXPECT_EQ(wait_status::failed, p.wait_for(milliseconds(1)));
    EXPECT_FALSE(p.terminate());
    EXPECT_FALSE(p.resume());
    EXPECT_FALSE(p.suspend());
}