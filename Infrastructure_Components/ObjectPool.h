#ifndef CPP_LEARN_OBJECTPOOL_H
#define CPP_LEARN_OBJECTPOOL_H

#include <iostream>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <stdexcept>

template<typename T>
class ObjectPool {
public:
    using ObjectPtr = std::shared_ptr<T>;

    explicit ObjectPool(size_t initialSize = 0) : maxSize_(0), count_(0) {
        if (initialSize > 0) {
            resize(initialSize);
        }
    }

    // 扩容池子
    void resize(size_t newSize) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (newSize < maxSize_) {
            throw std::invalid_argument("New size must be greater than current size");
        }
        for (size_t i = maxSize_; i < newSize; ++i) {
            pool_.push(new T());
            ++count_;
        }
        maxSize_ = newSize;
        cv_.notify_all(); // 唤醒可能等待的线程
    }

    // 获取对象
    ObjectPtr acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return !pool_.empty(); });

        T* raw = pool_.front();
        pool_.pop();
        return makeShared(raw);
    }

    // 当前池中可用对象数
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }

    // 当前对象总数（池内 + 使用中）
    size_t totalCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

private:
    // 包装 shared_ptr，释放时自动归还到池子
    ObjectPtr makeShared(T* raw) {
        return ObjectPtr(raw, [this](T* ptr) {
            std::unique_lock<std::mutex> lock(this->mutex_);
            pool_.push(ptr);
            lock.unlock();
            cv_.notify_one();
        });
    }

    size_t maxSize_;                      // 池容量上限
    size_t count_;                        // 总创建数量
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T*> pool_;                 // 存裸指针
};

namespace ObjectPool_Test {

class test_Obj {
public:
    test_Obj() { std::cout << "test_Obj()" << std::endl; }
    ~test_Obj() { std::cout << "~test_Obj()" << std::endl; }
    void print() { std::cout << "test_Obj::print()" << std::endl; }
};

void test() {
    ObjectPool<test_Obj> pool(2);

    auto obj1 = pool.acquire();
    obj1->print();

    auto obj2 = pool.acquire();
    obj2->print();

    std::cout << "pool size: " << pool.size() << std::endl;
    std::cout << "total count: " << pool.totalCount() << std::endl;

    // 扩容池子
    pool.resize(4);
    std::cout << "after resize to 4, pool size: " << pool.size() << std::endl;
    std::cout << "total count: " << pool.totalCount() << std::endl;

    auto obj3 = pool.acquire();
    auto obj4 = pool.acquire();
    obj3->print();
    obj4->print();

    obj1.reset();
    obj2.reset();
    obj3.reset();
    obj4.reset();

    std::cout << "after reset all, pool size: " << pool.size() << std::endl;
    std::cout << "total count: " << pool.totalCount() << std::endl;
}

}

#endif // CPP_LEARN_OBJECTPOOL_H
