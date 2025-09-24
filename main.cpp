#include <iostream>
#include "Concurrent_Control_Component/SpinLock.h"









int main() {

    std::ios::sync_with_stdio(false);
    std::cin.tie(0);
    std::cout.tie(0);
    SpinLock_Test::test();

    return 0;
}