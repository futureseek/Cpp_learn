#ifndef CPP_LEARN_SPINLOCK_H
#define CPP_LEARN_SPINLOCK_H

#include <atomic>
#include <chrono>
#include <thread>

class SpinLock{
private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void lock(){
        // 循环等待直到flag不是true
        while(flag.test_and_set(std::memory_order_acquire)){
            std::this_thread::yield(); // 让出CPU时间片，避免忙等待
        }
    }
    /*
        增加超时时间
    */
    bool try_lock_for(std::chrono::milliseconds timeout){
        auto end = std::chrono::steady_clock::now() + timeout;
        while(std::chrono::steady_clock::now() < end){
            if(!flag.test_and_set(std::memory_order_acquire)){
                return true; // 成功获取锁
            }
            std::this_thread::yield(); // 让出CPU时间片，避免忙等待
        }
        return false; // 超时未获取锁
    }

    void unlock()
    {
        flag.clear(std::memory_order_release);
    }
};

namespace SpinLock_Test{
    
    SpinLock splock;
    int shared_data = 0;

    void worker(){
        splock.lock();
        ++shared_data;
        std::cout<<"Thread " << std::this_thread::get_id() << " incremented shared_data to " << shared_data << std::endl;
        splock.unlock();
    }
    
    void test(){
        /*
            输出没有加锁，可能导致输出流交织
        */
        std::thread t1(worker);
        std::thread t2(worker);
        std::cout<< "t1 is id : "<<t1.get_id() << std::endl;
        std::cout<< "t2 is id : "<<t2.get_id() << std::endl;
        t1.join();
        t2.join();
        std::cout<<"Final shared_data: " << shared_data << std::endl;
    }
};

#endif //CPP_LEARN_SPINLOCK_H