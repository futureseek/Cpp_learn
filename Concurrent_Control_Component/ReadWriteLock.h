#ifndef CPP_LEARN_READWRITELOCK_H
#define CPP_LEARN_READWRITELOCK_H

#include <mutex>
#include <condition_variable>
#include <iostream>
#include <thread>
#include <vector>
#include <shared_mutex>

/*
    手动实现读写锁
*/
class ReadWriteLock {
private:
    std::mutex mtx_;
    std::condition_variable cv_;
    int readers_ = 0;
    bool writer_ = false;

public:
    void lock_read(){
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this](){return !writer_;});
        ++readers_;
    }

    void unlock_read(){
        std::unique_lock<std::mutex> lock(mtx_);
        --readers_;
        if(readers_ == 0){
            cv_.notify_all();
        }
    }

    void lock_write(){
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock,[this](){
            return !writer_ && readers_ == 0;
        });
        writer_ = true;
    }

    void unlock_write(){
        std::unique_lock<std::mutex> lock(mtx_);
        writer_ = false;
        cv_.notify_all();
    }

};

/*
    基于C++17标准库实现读写锁
*/
class ThreadSafeData{
private:
    mutable std::shared_mutex smtx_;
    int data_ = 0;
public:
    int read() const{
        std::shared_lock<std::shared_mutex> lock(smtx_);
        return data_;
    }
    void write(int value){
        std::unique_lock<std::shared_mutex> lock(smtx_);
        data_ = value;
    }
};


namespace ReadWriteLock_test{
    
    void test_shared_mutex(){
        ThreadSafeData data;
        std::vector<std::thread> threads;

        // 创建5个读者线程
        for (int i = 0; i < 5; ++i) {
            threads.emplace_back([&data, i] {
                std::this_thread::sleep_for(std::chrono::milliseconds(100 * i));
                std::cout << "Reader " << i << " sees: " << data.read() << "\n";
            });
        }
    
        // 创建2个写者线程
        for (int i = 0; i < 2; ++i) {
            threads.emplace_back([&data, i] {
                std::this_thread::sleep_for(std::chrono::milliseconds(50 * i));
                data.write(i + 10);
                std::cout << "Writer " << i << " updated value\n";
            });
        }
    
        for (auto& t : threads) t.join();
    }

    void test_manual_rwlock(){
        ReadWriteLock rwlock;
        int shared_data = 0;
        std::vector<std::thread> threads;

        // 创建5个读者线程
        for (int i = 0; i < 5; ++i) {
            threads.emplace_back([&rwlock, &shared_data, i] {
                std::this_thread::sleep_for(std::chrono::milliseconds(100 * i));
                rwlock.lock_read();
                std::cout << "Reader " << i << " sees: " << shared_data << "\n";
                rwlock.unlock_read();
            });
        }
    
        // 创建2个写者线程
        for (int i = 0; i < 2; ++i) {
            threads.emplace_back([&rwlock, &shared_data, i] {
                std::this_thread::sleep_for(std::chrono::milliseconds(50 * i));
                rwlock.lock_write();
                shared_data = i + 10;
                std::cout << "Writer " << i << " updated value\n";
                rwlock.unlock_write();
            });
        }
    
        for (auto& t : threads) t.join();
    }
    
    void test(){
        std::cout<<"---- test shared_mutex ----"<<std::endl;
        test_shared_mutex();

        std::cout<<"---- test manual rwlock ----"<<std::endl;
        test_manual_rwlock();
    }
};

#endif //CPP_LEARN_READWRITELOCK_H