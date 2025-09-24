#include <iostream>
#include "Concurrent_Control_Component/ReadWriteLock.h"









int main() {

    std::ios::sync_with_stdio(false);
    std::cin.tie(0);
    std::cout.tie(0);
    ReadWriteLock_test::test();

    return 0;
}