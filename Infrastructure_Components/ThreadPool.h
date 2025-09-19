#ifndef CPP_LEARN_THREADPOOL_H
#define CPP_LEARN_THREADPOOL_H

#include <vector>
#include <thread>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <iostream>

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop = false;
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
        for(size_t i = 0; i < num_threads; i++) {
            workers.emplace_back([this] {
                while(true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });

                        if(stop && tasks.empty()) {
                            return;
                        }

                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for(auto& worker : workers) {
            worker.join();
        }
    }

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));
        /*
         *  std::packaged_task用于包装一个函数/可调用对象，并将其执行结果与future绑定
         *  但是要求一个无参数的任务，所以用bind把f和参数列表提前绑定好
         *  使用shared_ptr是因为任务需要同时存在于任务队列和工作线程
         */
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> res = task->get_future();  // 修改这里
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if(stop) {
                throw std::runtime_error("ThreadPool is stopped");
            }
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }
};

// 测试代码
namespace ThreadPool_Test {
    static int add(int a, int b) {
        return a + b;
    }

    void test() {
        std::cout << "=== ThreadPool Test ===" << std::endl;

        ThreadPool pool(4);

        // 测试普通函数
        auto future1 = pool.enqueue(add, 2, 3);

        // 测试lambda表达式
        auto future2 = pool.enqueue([]() {
            return std::string("Hello from thread!");
        });

        std::cout << "2 + 3 = " << future1.get() << std::endl;
        std::cout << future2.get() << std::endl;

        // 测试多个并发任务
        std::cout << "Concurrent tasks: ";
        std::vector<std::future<int>> futures;
        for(int i = 0; i < 10; ++i) {
            futures.emplace_back(
                    pool.enqueue([i]() -> int {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        return i * i;
                    })
            );
        }

        for(auto& f : futures) {
            std::cout << f.get() << " ";
        }
        std::cout << std::endl;
    }
};

#endif //CPP_LEARN_THREADPOOL_H
