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
    static DWORD WINAPI SimpleRoutine(LPVOID lpParam) {
        if (lpParam) {
            DWORD sleepTime = static_cast<DWORD>(reinterpret_cast<uintptr_t>(lpParam));
            Sleep(sleepTime);
        }
        return 123;
    }

    static DWORD WINAPI SpinRoutine(LPVOID lpParam) {
        volatile bool* stop = static_cast<volatile bool*>(lpParam);
        while (!*stop) {
            YieldProcessor();
        }
        return 0;
    }

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
    EXPECT_FALSE(t.valid());
    EXPECT_FALSE(static_cast<bool>(t));
    EXPECT_FALSE(t.joinable());
    EXPECT_EQ(t.get_id(), 0u);
    EXPECT_EQ(t.handle(), nullptr);
}

TEST_F(ThreadTest, CreateAndJoin) {
    Thread t = CreateWorker(10);
    ASSERT_TRUE(t.valid());
    ASSERT_TRUE(t.joinable());
    
    t.wait(); 
    
    EXPECT_FALSE(t.joinable());
    auto exitCode = t.try_exit_code();
    ASSERT_TRUE(exitCode.has_value());
    EXPECT_EQ(exitCode.value(), 123u);
}

TEST_F(ThreadTest, HardwareConcurrency) {
    size_t cores = Thread::hardware_concurrency();
    EXPECT_GT(cores, 0u);
}

TEST_F(ThreadTest, MoveSemantics) {
    Thread t1 = CreateWorker(50);
    ASSERT_TRUE(t1.valid());
    HANDLE h = t1.handle();

    Thread t2 = std::move(t1);
    EXPECT_FALSE(t1.valid());
    EXPECT_TRUE(t2.valid());
    EXPECT_EQ(t2.handle(), h);

    t2.join();
}

TEST_F(ThreadTest, Detach) {
    Thread t = CreateWorker(0);
    ASSERT_TRUE(t.joinable());
    
    t.detach();
    EXPECT_FALSE(t.joinable());
    EXPECT_FALSE(t.valid());
}

TEST_F(ThreadTest, WaitStatusSignaled) {
    Thread t = CreateWorker(10);
    wait_status status = t.wait();
    EXPECT_EQ(status, wait_status::signaled);
    t.join();
}

TEST_F(ThreadTest, WaitForTimeout) {
    Thread t = CreateWorker(1000);
    
    wait_status status = t.wait_for(milliseconds(50));
    EXPECT_EQ(status, wait_status::timeout);
    
    EXPECT_TRUE(t.is_running());

    t.terminate(999);
    t.wait();
    
    auto code = t.try_exit_code();
    ASSERT_TRUE(code.has_value());
    EXPECT_EQ(code.value(), 999u);
    
    t.join();
}

TEST_F(ThreadTest, SuspendAndResume) {
    bool stop = false;
    Thread t = Thread::create(nullptr, 0, SpinRoutine, &stop, 0, nullptr);
    ASSERT_TRUE(t.valid());

    EXPECT_TRUE(t.suspend());
    EXPECT_TRUE(t.resume());
    
    stop = true;
    t.join();
}

TEST_F(ThreadTest, IsRunningAndExitCode) {
    Thread t = CreateWorker(100);
    
    EXPECT_TRUE(t.is_running());
    auto codeBefore = t.try_exit_code();

    EXPECT_FALSE(codeBefore.has_value()); 

    t.wait();
    
    EXPECT_FALSE(t.is_running());
    auto codeAfter = t.try_exit_code();
    ASSERT_TRUE(codeAfter.has_value());
    EXPECT_EQ(codeAfter.value(), 123u);

    t.join();
}

TEST_F(ThreadTest, ResetAndRelease) {
    Thread t = CreateWorker(0);
    HANDLE h = t.handle();
    ASSERT_NE(h, nullptr);

    HANDLE releasedH = t.release();
    EXPECT_EQ(h, releasedH);
    EXPECT_FALSE(t.valid());

    CloseHandle(releasedH);

    Thread t2;
    HANDLE hManual = CreateThread(nullptr, 0, SimpleRoutine, nullptr, CREATE_SUSPENDED, nullptr);
    t2.reset(hManual);
    EXPECT_TRUE(t2.valid());
    EXPECT_EQ(t2.handle(), hManual);
    
    t2.resume();
    t2.join();
}

TEST_F(ThreadTest, Swap) {
    Thread t1 = CreateWorker(0);
    Thread t2;

    HANDLE h1 = t1.handle();
    
    t1.swap(t2);

    EXPECT_FALSE(t1.valid());
    EXPECT_TRUE(t2.valid());
    EXPECT_EQ(t2.handle(), h1);

    t2.join();
}