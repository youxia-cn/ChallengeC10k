#include "SingletonThreadPool.hpp"

int main(){
    SingletonThreadPool& thread_pool = SingletonThreadPool::getThreadPool();
    for(int i=0; i<25; i++){
        thread_pool.commitTask(
                [=]{
                    std::cout << "i=" << i << " , thread_id=" << std::this_thread::get_id() << std::endl;
                }
            );
    }

    thread_pool.join();

    return 0;
}

