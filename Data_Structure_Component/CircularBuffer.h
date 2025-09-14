//
// Created by 12793 on 2025/9/14.
//

#ifndef CPP_LEARN_CIRCULARBUFFER_H
#define CPP_LEARN_CIRCULARBUFFER_H

#include <vector>
#include <mutex>
#include <condition_variable>

template <typename T>
class CircularBuffer{
private:
    std::vector<T> buf_;
    size_t max_size_;
    size_t head_ = 0;
    size_t tail_ = 0;
    bool full_ = false;
    mutable std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
public:
    explicit CircularBuffer(size_t size)
        : buf_(std::vector<T>(size+1)),
          max_size_(size+1),
          head_(0),
          tail_(0),
          full_(false){}
    CircularBuffer(const CircularBuffer&) = delete;
    CircularBuffer& operator=(const CircularBuffer&) = delete;
    bool empty() const {
        return (!full_ && (head_ == tail_));
    }
    bool full() const {
        return ((tail_+1)%max_size_) == head_;
    }
    size_t capacity() const {
        return max_size_ - 1;
    }
    size_t size() const{
        if(full_){
            return capacity();
        }
        return (tail_>=head_)?(tail_-head_):(max_size_-head_+tail_);
    }
    void push(const T& item){
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait(lock,[this](){
            return !full();
        });

        buf_[tail_] = item;
        tail_=(tail_+1)%max_size_;

        lock.unlock();
        not_empty_.notify_one();
    }
    T pop(){
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock,[this](){
            return !empty();
        });

        T item = buf_[head_];
        head_ = (head_+1)%max_size_;

        lock.unlock();
        not_full_.notify_one();
        return item;
    }
    T front() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (empty()) {
            throw std::runtime_error("Buffer is empty");
        }
        return buf_[head_];
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        head_ = tail_;
        full_ = false;
    }
};


namespace CircularBuffer_Test{
    void test()
    {
        CircularBuffer<int> cb(5);  // 创建容量为5的环形缓冲区
        // 填充缓冲区
        for (int i = 0; i < 5; ++i) {
            cb.push(i);
            std::cout << "Pushed: " << i << ", Size: " << cb.size() << std::endl;
        }
        // 此时缓冲区已满
        std::cout << "Buffer is full? " << std::boolalpha << cb.full() << std::endl;
        // 取出元素
        for (int i = 0; i < 3; ++i) {
            int val = cb.pop();
            std::cout << "Popped: " << val << ", Size: " << cb.size() << std::endl;
        }
        // 再次填充
        for (int i = 5; i < 8; ++i) {
            cb.push(i);
            std::cout << "Pushed: " << i << ", Size: " << cb.size() << std::endl;
        }
        // 清空缓冲区
        cb.clear();
        std::cout << "After clear, size: " << cb.size() << std::endl;
    }
}


#endif //CPP_LEARN_CIRCULARBUFFER_H
