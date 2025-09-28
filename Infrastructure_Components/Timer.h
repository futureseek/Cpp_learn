#ifndef CPP_LEARN_TIMER_H
#define CPP_LEARN_TIMER_H


#include <chrono>
#include <functional>
#include <thread>
#include <queue>
#include <algorithm>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <atomic>

class Timer{
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration; // 存储时间间隔
    using Task = std::function<void()>;

    struct TimerTask{
        TimePoint next_run; // 下次执行时间
        Duration interval; // 多次任务间隔，0表示一次性任务
        Task task;
        uint64_t id;

        bool operator>(const TimerTask& other) const {
            return next_run > other.next_run;
        }
    };
private:
    std::priority_queue<TimerTask,std::vector<TimerTask>,std::greater<TimerTask>> heap_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> next_id_{1};
    std::thread worker_thread_;

public:
    Timer() = default;
    ~Timer(){
        stop();
    }

    void start(){
        if(running_.exchange(true)){
            //exchange读取running_的值，并将其设置为true，返回旧值，如果之前是false就不会进入if
            return;
        }
        worker_thread_ = std::thread([this]{
            run();
        });
    }

    // 添加一次性任务
    uint64_t add_one_shot_task(Duration delay,Task task){
        return add_task(delay,Duration::zero(),std::move(task));
    }
    // 添加周期性任务
    uint64_t add_fixed_rate_task(Duration inital_delay,Duration interval,Task task){
        return add_task(inital_delay,interval,std::move(task));
    }
    bool cancel_task(uint64_t task_id){
        // TODO
        return false;
    }

    void stop()
    {
        if(!running_.exchange(false)){
            return;
        }
        cv_.notify_all();
        if(worker_thread_.joinable()){
            worker_thread_.join();
        }
    }

private:
    void run()
    {
        while(running_){
            std::unique_lock<std::mutex> lock(mutex_);
            if(heap_.empty()){
                cv_.wait(lock,[this](){
                    return !running_ || !heap_.empty();
                });
                continue; // continue的作用，重新校验状态
            }
            const auto& top_task = heap_.top();
            auto now = Clock::now();
            if(top_task.next_run > now){
                cv_.wait_until(lock,top_task.next_run);
                continue;
            }
            auto task = heap_.top();
            heap_.pop();
            lock.unlock();
            try{
                task.task();
            }
            catch(...){
                // TODO
            }
            if(task.interval > Duration::zero()){
                lock.lock();
                TimerTask new_task{
                    Clock::now() + task.interval,
                    task.interval,
                    std::move(task.task),
                    task.id
                };
                heap_.push(std::move(new_task));
                cv_.notify_one();
            }
        }
    }

    uint64_t add_task(Duration delay,Duration interval,Task task){
        auto id=next_id_.fetch_add(1);
        auto now = Clock::now();
        TimerTask timer_task{
            now+delay,
            interval,
            std::move(task),
            id
        };
        {
            std::lock_guard<std::mutex> lock(mutex_);
            heap_.push(std::move(timer_task));
        }
        cv_.notify_one();
        return id;
    }
};

namespace Timer_Test{
    void test(){
        Timer timer;
        timer.start();

        timer.add_one_shot_task(std::chrono::seconds(2),[]{
            std::cout<<"One shot task executed after 2 seconds"<<std::endl;
        });

        timer.add_fixed_rate_task(std::chrono::seconds(1),std::chrono::seconds(3),[]{
            static int count =0;
            std::cout<<"Fixed rate task executed "<<++count<<" times"<<std::endl;
        });

        std::this_thread::sleep_for(std::chrono::seconds(10));
        timer.stop();
    }
};

#endif //CPP_LEARN_TIMER_H