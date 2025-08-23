//
// Created by 12793 on 2025/7/31.
//

#ifndef CLION_TEST_TIMEWHEEL_H
#define CLION_TEST_TIMEWHEEL_H
#include <iostream>
#include <vector>
#include <atomic>
#include <functional>
#include <thread>
#include <memory>
#include <unordered_map>


namespace TimeWheel{

// 时间轮任务节点
struct TimerTask{
    uint64_t id;
    uint64_t expire_time; //到期时间
    std::function<void()> callback;
    std::shared_ptr<TimerTask> next;

    TimerTask(uint64_t task_id,uint64_t e_time,std::function<void()> cb):id(task_id),expire_time(e_time),callback(std::move(cb)),next(
            nullptr){}
};

// 时间轮实现
class TimeWheel{
public:
    TimeWheel():wheel(WHEEL_SIZE),current_tick(0),next_task_id(1),running(false){}

    ~TimeWheel() {
        stop();
    }
    // 启动
    void start(){
        if(!running.load()){
            running.store(true);
            worker_thread = std::thread(&TimeWheel::worker_loop,this);
            std::cout<<" timewheel start "<<std::endl;
        }
    }
    // 停止
    void stop(){
        if(running.load()){
            running.store(false);
            if(worker_thread.joinable()){
                worker_thread.join();
            }
            std::cout<<" timewheel stop"<<std::endl;
        }
    }
    // 添加定时任务
    uint64_t add_timer(uint32_t delay_ms,std::function<void()> callback){
        uint64_t expire_tick = current_tick + (delay_ms/TICK_MS) ;
        uint64_t task_id = next_task_id++;

        auto task =std::make_shared<TimerTask>(task_id,expire_tick,std::move(callback));

        //计算槽位
        int slot = expire_tick % WHEEL_SIZE;

        // 插入对应槽位的任务链表头
        task->next = wheel[slot];
        wheel[slot] =task;

        task_map[task_id] = task;

        return task_id;
    }
    //取消定时任务
    bool cancel_timer(uint64_t task_id){
        auto it=task_map.find(task_id);
        if(it!=task_map.end()){
            it->second->callback= nullptr;
            task_map.erase(it);
            return true;
        }
        return false;
    }
    // 推进一个tick
    void step(){
        int slot = current_tick % WHEEL_SIZE;
        auto& head = wheel[slot];
        // 处理所有任务
        auto current =head;
        head = nullptr;
        while(current){
            auto next=current->next;
            // 任务到期就执行，否则就重新插回
            if(current->expire_time<=current_tick){
                if(current->callback){
                    try{
                        current->callback();
                    }catch(const std::exception& e){
                        std::cerr<<"Timer task exception : "<<e.what()<<std::endl;
                    }
                }

                task_map.erase(current->id);
            }
            else{
                int new_slot = current->expire_time%WHEEL_SIZE;
                current->next =wheel[new_slot];
                wheel[new_slot]=current;
            }

            current=next;
        }


        current_tick++;
    }
    uint64_t get_current_time_ms(){
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
    // 工作线程函数
    void worker_loop(){
        uint64_t last_time =get_current_time_ms();

        while(running.load()){
            uint64_t current_time = get_current_time_ms();

            uint64_t elapsed = current_time-last_time;
            uint64_t ticks_to_advance = elapsed / TICK_MS;

            for(uint64_t i=0;i<ticks_to_advance;++i){
                step();
            }
            last_time = current_time;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    // 获取当前活跃任务数量
    size_t get_active_task_count() const {
        return task_map.size();
    }

    // 获取当前tick
    uint64_t get_current_tick() const {
        return current_tick;
    }
private:
    static const int WHEEL_SIZE = 256;// 时间轮槽数
    static const int TICK_MS = 10; //每个TICK的毫秒数

    std::vector<std::shared_ptr<TimerTask>> wheel; // 时间轮槽位
    std::unordered_map<uint64_t, std::shared_ptr<TimerTask>> task_map;  // 任务映射
    uint64_t current_tick;
    uint64_t next_task_id;
    std::atomic<bool> running; // 运行状态
    std::thread worker_thread; // 工作线程


};



}


#endif //CLION_TEST_TIMEWHEEL_H
