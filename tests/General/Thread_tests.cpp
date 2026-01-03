/**
 * @file Thread_tests.cpp
 * @brief Unit tests for the Thread RAII wrapper using GoogleTest.
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
#include <optional>
#include <chrono>

#include <core/General/Thread.h>

using namespace core::General;

class ThreadTest : public ::testing::Test {
protected:
    /**
     * Standard thread entry point that sleeps and returns a fixed exit code.
     */
    static DWORD WINAPI SimpleRoutine(LPVOID lpParam) {
        if (lpParam) {
            DWORD sleepTime = static_cast<DWORD>(reinterpret_cast<uintptr_t>(lpParam));
            Sleep(sleepTime);
        }
        return 123;
    }

    /**
     * Busy-wait thread routine used for testing suspend/resume functionality.
     */
    static DWORD WINAPI SpinRoutine(LPVOID lpParam) {
        volatile bool* stop = static_cast<volatile bool*>(lpParam);
        while (!*stop) {
            // Low-level hint to the CPU that we are in a spin loop
            YieldProcessor();
        }
        return 0;
    }

    /**
     * Helper to spawn a worker thread via the class factory.
     */
    Thread CreateWorker(DWORD sleepMs = 0) {
        return Thread::create(
            nullptr, 
            0, 
            SimpleRoutine, 
            reinterpret_cast<LPVOID>(static_cast<uintptr_t>(sleepMs)), 
            0, 
            nullptr
        );
    }
};

TEST_F(ThreadTest, DefaultCtorIsInvalid) {
    Thread t;
    // An uninitialized thread must report as invalid and have no system IDs
    EXPECT_FALSE(t.valid());
    EXPECT_FALSE(static_cast<bool>(t));
    EXPECT_FALSE(t.joinable());
    EXPECT_EQ(0u, t.get_id());
    EXPECT_EQ(nullptr, t.handle());
}

TEST_F(ThreadTest, CreateAndJoin) {
    Thread t = CreateWorker(10);
    ASSERT_TRUE(t.valid());
    ASSERT_TRUE(t.joinable());

    // We wait first to ensure completion while keeping the handle open for inspection
    t.wait(); 
    
    ASSERT_TRUE(t.joinable()); 
    
    auto exitCode = t.try_exit_code();
    ASSERT_TRUE(exitCode.has_value());
    EXPECT_EQ(123u, exitCode.value());

    // join() performs the final handle closure (reset)
    t.join();

    EXPECT_FALSE(t.joinable());
    EXPECT_FALSE(t.valid());
}

TEST_F(ThreadTest, HardwareConcurrency) {
    // Basic system check: should always report at least 1 logical core
    size_t cores = Thread::hardware_concurrency();
    EXPECT_GT(cores, 0u);
}

TEST_F(ThreadTest, MoveSemantics) {
    Thread t1 = CreateWorker(50);
    ASSERT_TRUE(t1.valid());
    HANDLE h = t1.handle();

    // t2 takes ownership of the kernel handle; t1 is nullified
    Thread t2 = std::move(t1);
    EXPECT_FALSE(t1.valid());
    EXPECT_TRUE(t2.valid());
    EXPECT_EQ(h, t2.handle());

    t2.join();
}

TEST_F(ThreadTest, Detach) {
    Thread t = CreateWorker(0);
    ASSERT_TRUE(t.joinable());
    
    // Detach should close the handle locally but let the thread finish in the background
    t.detach();
    EXPECT_FALSE(t.joinable());
    EXPECT_FALSE(t.valid());
}

TEST_F(ThreadTest, WaitStatusSignaled) {
    Thread t = CreateWorker(10);
    // Blocking wait until the thread kernel object transitions to a signaled state
    wait_status status = t.wait();
    EXPECT_EQ(wait_status::signaled, status);
    t.join();
}

TEST_F(ThreadTest, WaitForTimeout) {
    Thread t = CreateWorker(1000); // 1 second task
    
    // 50ms wait is expected to expire (timeout)
    wait_status status = t.wait_for(milliseconds(50));
    EXPECT_EQ(wait_status::timeout, status);
    
    EXPECT_TRUE(t.is_running());

    // Force termination and verify the provided exit code (999)
    t.terminate(999);
    t.wait();
    
    auto code = t.try_exit_code();
    ASSERT_TRUE(code.has_value());
    EXPECT_EQ(999u, code.value());
    
    t.join();
}

TEST_F(ThreadTest, SuspendAndResume) {
    bool stop = false;
    Thread t = Thread::create(nullptr, 0, SpinRoutine, &stop, 0, nullptr);
    ASSERT_TRUE(t.valid());

    // Verify thread control calls don't return error status
    EXPECT_TRUE(t.suspend());
    EXPECT_TRUE(t.resume());
    
    // Signal the spin routine to exit
    stop = true;
    t.join();
}

TEST_F(ThreadTest, IsRunningAndExitCode) {
    Thread t = CreateWorker(100);
    
    EXPECT_TRUE(t.is_running());
    auto codeBefore = t.try_exit_code();

    // try_exit_code must return nullopt while the OS reports the thread as active
    EXPECT_FALSE(codeBefore.has_value()); 

    t.wait();
    
    EXPECT_FALSE(t.is_running());
    auto codeAfter = t.try_exit_code();
    ASSERT_TRUE(codeAfter.has_value());
    EXPECT_EQ(123u, codeAfter.value());

    t.join();
}

TEST_F(ThreadTest, ResetAndRelease) {
    Thread t = CreateWorker(0);
    HANDLE h = t.handle();
    ASSERT_NE(nullptr, h);

    // release() gives the handle to the caller; destructor will no longer close it
    HANDLE releasedH = t.release();
    EXPECT_EQ(h, releasedH);
    EXPECT_FALSE(t.valid());

    // Manual cleanup required for released handles
    CloseHandle(releasedH);

    // reset() allows reuse of an existing Thread object with a new handle
    Thread t2;
    HANDLE hManual = CreateThread(nullptr, 0, SimpleRoutine, nullptr, CREATE_SUSPENDED, nullptr);
    t2.reset(hManual);
    EXPECT_TRUE(t2.valid());
    EXPECT_EQ(hManual, t2.handle());
    
    t2.resume();
    t2.join();
}

TEST_F(ThreadTest, Swap) {
    Thread t1 = CreateWorker(0);
    Thread t2;

    HANDLE h1 = t1.handle();
    
    // Internal state swap
    t1.swap(t2);

    EXPECT_FALSE(t1.valid());
    EXPECT_TRUE(t2.valid());
    EXPECT_EQ(h1, t2.handle());

    t2.join();
}