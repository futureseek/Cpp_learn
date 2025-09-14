//
// Created by 12793 on 2025/9/11.
//

#ifndef CPP_LEARN_LOCKFREEQUEUE_H
#define CPP_LEARN_LOCKFREEQUEUE_H

#include <atomic>
#include <memory>
#include <iostream>

template<typename T>
struct LockFreeQueueNode{
    std::atomic<T*> data;
    std::atomic<LockFreeQueueNode<T>*> next;

    LockFreeQueueNode():data(nullptr),next(nullptr){}
};
template<typename T>
class LockFreeQueue{
private:
    std::atomic<LockFreeQueueNode<T>*> head;
    std::atomic<LockFreeQueueNode<T>*> tail;
public:
    LockFreeQueue(){
        LockFreeQueueNode<T>* dummy = new LockFreeQueueNode<T>();
        head.store(dummy);
        tail.store(dummy);
    }
    ~LockFreeQueue(){
        while(head.load()!= nullptr){
            LockFreeQueueNode<T>* node = head.load();
            head.store(node->next.load());
            delete node;
        }
    }

    void enqueue(T* item){
        LockFreeQueueNode<T>* node = new LockFreeQueueNode<T>();
        node->data.store(item);

        LockFreeQueueNode<T>* prev_tail = tail.load();
        // 循环等待
        while(1){
            LockFreeQueueNode<T>* nxt = prev_tail->next.load();
            if(prev_tail==tail.load()){
                // 双重检查
                if(nxt == nullptr){
                    if(prev_tail->next.compare_exchange_weak(nxt,node)){
                        break;
                    }
                }
                else{
                    //next不为nullptr说明有其他线程已经插入了节点
                    tail.compare_exchange_weak(prev_tail,nxt);
                }
            }
        }
        // 最终更新
        tail.compare_exchange_weak(prev_tail,node);
    }

    T* dequeue(){
        while(true){
            LockFreeQueueNode<T>* first = head.load();
            LockFreeQueueNode<T>* last = tail.load();
            LockFreeQueueNode<T>* next = first->next.load();

            if(first == head.load()){
                if(first == last){
                    if(next == nullptr){
                        return nullptr;// 队列空
                    }
                    //队列不为空，但是tail滞后
                    tail.compare_exchange_weak(last,next);
                }
                else{
                    T* result = next->data.load();
                    if(head.compare_exchange_weak(first,next)){
                        delete first;
                        return result;
                    }
                }
            }
        }
    }

    bool empty()
    {
        LockFreeQueueNode<T>* first = head.load();
        LockFreeQueueNode<T>* next = first->next.load();
        return next == nullptr;
    }

};

namespace LockFreeQueue_Test{
    void test(){
        LockFreeQueue<int> queue;

        // 测试空队列
        std::cout << "empty queue test: " << (queue.empty() ? "true" : "false") << std::endl;

        // 测试入队
        std::cout<<"input sth"<<std::endl;
        int* val1 = new int(10);
        int* val2 = new int(20);
        int* val3 = new int(30);

        queue.enqueue(val1);
        queue.enqueue(val2);
        queue.enqueue(val3);

        std::cout << "empty test: " << (!queue.empty() ? "true" : "false") << std::endl;

        // 测试出队
        int* result1 = queue.dequeue();
        int* result2 = queue.dequeue();
        int* result3 = queue.dequeue();

        std::cout << "dequeue result: " << *result1 << " " << *result2 << " " << *result3 << std::endl;
        std::cout << "stand_result: 10 20 30" << std::endl;

        // 测试空队列
        int* result4 = queue.dequeue();
        std::cout << "empty queue dequeue: " << (result4 == nullptr ? "true" : "false") << std::endl;
        std::cout << "test: " << (queue.empty() ? "true" : "false") << std::endl;

        // 清理内存
        delete result1;
        delete result2;
        delete result3;

        std::cout << "end" << std::endl << std::endl;
    }
};



#endif //CPP_LEARN_LOCKFREEQUEUE_H
