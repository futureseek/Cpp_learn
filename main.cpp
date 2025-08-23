#include <iostream>
#include "LRU.h"




void test()
{
    LRUCache cache(2);

    cache.put(1, 1); // 缓存现在为 {1=1}
    cache.put(2, 2); // 缓存现在为 {1=1, 2=2}
    std::cout << "Get 1: " << cache.get(1) << std::endl; // 返回 1
    cache.put(3, 3); // 缓存达到容量，移除最近最少使用的键 2，缓存现在为 {1=1, 3=3}
    std::cout << "Get 2: " << cache.get(2) << std::endl; // 返回 -1（未找到）
    cache.put(4, 4); // 缓存达到容量，移除最近最少使用的键 1，缓存现在为 {3=3, 4=4}
    std::cout << "Get 1: " << cache.get(1) << std::endl; // 返回 -1（未找到）
    std::cout << "Get 3: " << cache.get(3) << std::endl; // 返回 3
    std::cout << "Get 4: " << cache.get(4) << std::endl; // 返回 4
}


int main() {

    std::ios::sync_with_stdio(false);
    std::cin.tie(0);
    std::cout.tie(0);

    test();


    return 0;
}