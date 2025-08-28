//
// Created by 12793 on 2025/8/27.
//

#ifndef CPP_LEARN_THREADPOOL_H
#define CPP_LEARN_THREADPOOL_H
#include <iostream>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <future>
#include <memory>

class ThreadPool{
private:
    // 任务队列
    std::queue<std::function<void()>> tasks_;

    // 同步原语
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;

    // 工作线程
    std::vector<std::thread> workers_;
public:
    explicit ThreadPool(size_t thread_count = std::thread::hardware_concurrency()):stop_(false){
        for(size_t i=0;i<thread_count;++i){
            workers_.emplace_back([this]{
               for(;;){
                   std::function<void()> task;
                   // 等待任务
                   {
                        std::unique_lock<std::mutex> lock(this->queue_mutex_);
                        this->condition_.wait(lock,[this]{
                            return this->stop_ || !this->tasks_.empty();
                        });

                        if(this->stop_ && this->tasks_.empty()){
                            return;
                        }

                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                   }
                   task();
               }
            });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool() {
        stop_ = false;
        condition_.notify_all();
        for(std::thread& worker : workers_){
            if(worker.joinable()){
                worker.join();
            }
        }
    }

    template<class F,class... Args>
    auto submit(F&& f,Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        // 推导返回类型
        using return_type = typename std::result_of<F(Args...)>::type;

        // 创建打包任务
        auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f),std::forward<Args>(args)...)
                );
        std::future<return_type> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            if(stop_){
                throw std::runtime_error("pool is stop");
            }

            tasks_.template emplace([task](){
                (*task)();
            });
        }


        condition_.notify_one();

        return result;
    }

    size_t thread_count() const {
        return workers_.size();
    }

    size_t task_count() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }
};

namespace ThreadPool_Test{
    void test()
    {
        try {
            // 创建线程池
            ThreadPool pool(4);

            // 提交一些任务
            std::vector<std::future<int>> results;

            for (int i = 0; i < 8; ++i) {
                results.emplace_back(
                        pool.submit([i] {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            return i * i;
                        })
                );
            }

            // 获取结果
            for (auto& result : results) {
                std::cout << result.get() << " ";
            }
            std::cout << std::endl;

            // 提交无返回值的任务
            auto future = pool.submit([]() {
                std::cout << "Hello from thread pool!" << std::endl;
            });
            future.wait();

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
};

#endif //CPP_LEARN_THREADPOOL_H
