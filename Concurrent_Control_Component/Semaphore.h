#ifndef CPP_LEARN_SEMAHPRE_H
#define CPP_LEARN_SEMAHPRE_H

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

class Semaphore{
private:
    std::mutex mtx_;
    std::condition_variable cv_;
    int count_;
public:
    explicit Semaphore(int initial=1):count_(initial){}
    void acquire(){
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock,[this](){
            return count_>0;
        });
        --count_;
    }
    void release(){
        std::lock_guard<std::mutex> lock(mtx_);
        ++count_;
        cv_.notify_one();
    }
    bool try_acquire(){
        std::lock_guard<std::mutex> lock(mtx_);
        if(count_>0){
            --count_;
            return true;
        }
        return false;
    }
    // 超时
    bool try_acquire_for(std::chrono::milliseconds timeout){
        std::unique_lock<std::mutex> lock(mtx_);
        if(!cv_.wait_for(lock,timeout,[this](){
            return count_>0;
        })){
            return false;
        }
        --count_;
        return true;
    }
};

namespace Semaphore_Test {
    // 测试基础 acquire/release
    void test_basic() {
        Semaphore sem(2);
        std::cout << "[BASIC TEST] Start (count=2)\n";
        sem.acquire();
        std::cout << "  - Thread 1 acquired\n";
        sem.acquire();
        std::cout << "  - Thread 2 acquired\n";
        sem.release();
        std::cout << "  - Thread 1 released\n";
        sem.release();
        std::cout << "  - Thread 2 released\n";
        std::cout << "[BASIC TEST] Passed\n\n";
    }
    // 测试 try_acquire
    void test_try_acquire() {
        Semaphore sem(1);
        std::cout << "[TRY_ACQUIRE TEST] Start\n";
        if (sem.try_acquire()) {
            std::cout << "  - First try_acquire: success (expected)\n";
        }
        if (!sem.try_acquire()) {
            std::cout << "  - Second try_acquire: failed (expected)\n";
        }
        sem.release();
        if (sem.try_acquire()) {
            std::cout << "  - After release: success (expected)\n";
        }
        std::cout << "[TRY_ACQUIRE TEST] Passed\n\n";
    }
    // 测试超时功能
    void test_timeout() {
        Semaphore sem(1);
        std::cout << "[TIMEOUT TEST] Start\n";
        sem.acquire(); // 占用信号量
        auto start = std::chrono::steady_clock::now();
        bool result = sem.try_acquire_for(std::chrono::milliseconds(500));
        auto end = std::chrono::steady_clock::now();
        if (!result) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "  - Timeout after ~" << duration.count() << "ms (expected)\n";
        }
        sem.release();
        result = sem.try_acquire_for(std::chrono::milliseconds(100));
        if (result) {
            std::cout << "  - Acquired immediately after release (expected)\n";
        }
        std::cout << "[TIMEOUT TEST] Passed\n\n";
    }
    // 并发测试（多个线程竞争信号量）
    void test_concurrency() {
        const int kThreads = 5;
        Semaphore sem(3); // 允许3个并发
        std::vector<std::thread> threads;
        std::atomic<int> concurrent(0);
        std::atomic<int> max_concurrent(0);
        std::cout << "[CONCURRENCY TEST] Starting " << kThreads << " threads (max 3 concurrent)\n";
        for (int i = 0; i < kThreads; ++i) {
            threads.emplace_back([&sem, &concurrent, &max_concurrent, i]() {
                sem.acquire();
                int current = ++concurrent;
                int old_max = max_concurrent.load();
                while (old_max < current && !max_concurrent.compare_exchange_weak(old_max, current)) {}
                std::cout << "  - Thread " << i << " entered (" << current << " concurrent)\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                --concurrent;
                sem.release();
                std::cout << "  - Thread " << i << " exited\n";
            });
        }
        for (auto& t : threads) {
            t.join();
        }
        std::cout << "  - Maximum observed concurrent threads: " << max_concurrent << " (expected <=3)\n";
        if (max_concurrent <= 3) {
            std::cout << "[CONCURRENCY TEST] Passed\n\n";
        } else {
            std::cerr << "[CONCURRENCY TEST] FAILED! (max_concurrent=" << max_concurrent << ")\n";
        }
    }
    // 综合测试入口
    void test() {
        std::cout << "===== Semaphore Test Begin =====\n";
        test_basic();
        test_try_acquire();
        test_timeout();
        test_concurrency();
        std::cout << "===== All Tests Passed! =====\n";
    }
}

#endif //CPP_LEARN_SEMAPHORE_H