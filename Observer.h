//
// Created by 12793 on 2025/8/24.
//

#ifndef CPP_LEARN_OBSERVER_H
#define CPP_LEARN_OBSERVER_H

#include <iostream>
#include <vector>
#include <string>


class Observer{
public:
    virtual ~Observer() = default;
    virtual void update(const std::string& message) = 0;
};

class Subject{
protected:
    std::vector<std::weak_ptr<Observer>> observers;
public:
    virtual ~Subject() = default;

    /*
     * 传入的时候使用shared_ptr 保证在attach期间是有效的，但是在保存的时候是weak_ptr，防止循环引用
     */
    void attach(std::shared_ptr<Observer> observer){
        observers.push_back(observer);
    }

    void detach(std::shared_ptr<Observer> observer){
        auto it = std::find_if(observers.begin(),observers.end(),
                               [&observer](const std::weak_ptr<Observer>& wp){
            auto sp = wp.lock();
            return sp && sp == observer;
        });
        if(it != observers.end()){
            observers.erase(it);
        }
    }

    void notify(const std::string& message){
        observers.erase(std::remove_if(observers.begin(),observers.end(),[](const std::weak_ptr<Observer>& wp){
            return wp.expired();
        }),observers.end());

        for(const auto& weak_ptr:observers){
            if(auto observer = weak_ptr.lock()){
                observer->update(message);
            }
        }
    }
};

// 具体主题 - 新闻发布者
class NewsPublisher : public Subject {
private:
    std::string latestNews;

public:
    void setNews(const std::string& news) {
        latestNews = news;
        std::cout << "output: " << latestNews << std::endl;
        notify(latestNews);
    }

    std::string getLatestNews() const {
        return latestNews;
    }
};
// 具体观察者 - 新闻订阅者
class NewsSubscriber : public Observer {
private:
    std::string name;

public:
    explicit NewsSubscriber(const std::string& subscriberName)
            : name(subscriberName) {}

    void update(const std::string& message) override {
        std::cout << name << " rec: " << message << std::endl;
    }
};
class MobileAppSubscriber : public Observer {
private:
    std::string appName;

public:
    explicit MobileAppSubscriber(const std::string& appName)
            : appName(appName) {}

    void update(const std::string& message) override {
        std::cout << "app: " << appName << ": rec - "
                  << message << std::endl;
    }
};

namespace Observer_Test{
    void test()
    {
        auto publisher = std::make_shared<NewsPublisher>();

        // 创建订阅者
        auto subscriber1 = std::make_shared<NewsSubscriber>("first");
        auto subscriber2 = std::make_shared<NewsSubscriber>("second");
        auto mobileApp = std::make_shared<MobileAppSubscriber>("third");

        // 添加订阅者
        publisher->attach(subscriber1);
        publisher->attach(subscriber2);
        publisher->attach(mobileApp);

        std::cout<<"firsst test:"<<std::endl;
        publisher->setNews("first output");

        publisher->detach(subscriber1);

        std::cout<<"sec test:"<<std::endl;
        publisher->setNews("sec output");

    }
};

#endif //CPP_LEARN_OBSERVER_H
