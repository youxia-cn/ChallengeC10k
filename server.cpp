#include "SingletonThreadPool.hpp"
#include <iostream>

int main(){

    std::cout << "Hello, World!" << std::endl;

    SingletonThreadPool& thread_pool = SingletonThreadPool::getThreadPool();
    std::vector<int> vec;
    vec.push_back(10);

    return 0;

}
