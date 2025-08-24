//
// Created by 12793 on 2025/8/24.
//

#ifndef CPP_LEARN_SINGLETON_H
#define CPP_LEARN_SINGLETON_H

#include <iostream>

class Singleton{
private:
    Singleton(){
        std::cout<<"init"<<std::endl;
    }
public:
    static Singleton& getInstance()
    {
        static Singleton instance;
        return instance;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};


namespace Singleton_test{
    void test()
    {
        Singleton& s1 = Singleton::getInstance();
        Singleton& s2 = Singleton::getInstance();

        std::cout<<(&s1)<<" "<<(&s2)<<std::endl;
    }
};

#endif //CPP_LEARN_SINGLETON_H
