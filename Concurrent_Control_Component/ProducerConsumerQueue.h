#ifndef CPP_LEARN_PRODUCERCONSUMERQUEUE_H
#define CPP_LEARN_PRODUCERCONSUMERQUEUE_H


#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>


template<typename T>
class ProducerConsumerQueue{
private:
    std::queue<T> queue_;
    mutable std::mutex mtx_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    size_t max_size_;
    bool shutdown_ = false;
public:
    explicit ProducerConsumerQueue(size_t max_size=0):max_size_(max_size){}
    ProducerConsumerQueue(const ProducerConsumerQueue&) = delete;
    ProducerConsumerQueue& operator=(const ProducerConsumerQueue&) = delete;

    void push(const T& item){
        std::unique_lock<std::mutex> lock(mtx_);
        if(max_size_>0){
            not_full_.wait(lock,[this](){
                return queue_.size()<max_size_ || shutdown_;
            });
        }
        if(shutdown_){
            throw std::runtime_error("the queue is shutting down");
        }
        queue_.push(item);
        not_empty_.notify_one();
    }

    void push(T&& item){
        std::unique_lock<std::mutex> lock(mtx_);
        if(max_size_>0){
            not_full_.wait(lock,[this](){
                return queue_.size()<max_size_ || shutdown_;
            });
        }
        if(shutdown_){
            throw std::runtime_error("the queue is shutting down");
        }
        queue_.push(move(item));
        not_empty_.notify_one();
    }

    /*
        pop 用unique_lock
        try_pop 用lock_guard
        因为pop中间会短暂释放锁，阻塞等待通知，所以需要灵活一点的RAII锁
    */
    bool pop(T& value){
        std::unique_lock<std::mutex> lock(mtx_);
        not_empty_.wait(lock,[this](){
            return !queue_.empty() || shutdown_;
        });
        if(shutdown_ && queue_.empty()){
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop();
        if(max_size_>0){
            not_full_.notify_one();
        }
        return true;
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.empty()) {
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop();
        if (max_size_ > 0) {
            not_full_.notify_one();
        }
        return true;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }
    // 关闭队列，唤醒所有等待线程
    void shutdown() {
        std::lock_guard<std::mutex> lock(mtx_);
        shutdown_ = true;
        not_empty_.notify_all();
        not_full_.notify_all();
    }
};

namespace ProDucerConsumerQueue_Test{
    void test()
    {
        ProducerConsumerQueue<int> queue(10);  // 容量为10的队列
        std::atomic<bool> done(false);
        // 生产者线程
        std::thread producer([&]() {
            for (int i = 0; i < 20; ++i) {
                queue.push(i);
                std::cout << "Produced: " << i << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            // 生产完成后关闭队列
            queue.shutdown();
        });
        // 消费者线程
        std::thread consumer([&]() {
            while (true) {
                int value;
                if (queue.pop(value)) {
                    std::cout << "Consumed: " << value << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(150));
                } else {
                    // 队列已关闭且为空才会返回false
                    break;
                }
            }
        });
        producer.join();
        consumer.join();
        std::cout << "Producer-consumer test completed." << std::endl;
    }
};

#endif //CPP_LEARN_PRODUCERCONSUMERQUEUE_H