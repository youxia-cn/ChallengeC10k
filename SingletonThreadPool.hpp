#ifndef SINGLETON_THREAD_POOL_HPP
#define SINGLETON_THREAD_POOL_HPP

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <iostream>

class SingletonThreadPool{
    static SingletonThreadPool* _tp;

    std::queue<std::function<void()>> taskQueue;
    std::mutex mut;
    std::condition_variable cond;
    std::vector<std::thread> threads;
    bool done=false;

    public:
    static SingletonThreadPool& getThreadPool(){
        if(_tp == nullptr){
            _tp = new SingletonThreadPool(20);
        }
        return *_tp;
    }

    SingletonThreadPool(int thread_amount){
        for(int i=0; i<thread_amount; i++){
            std::thread t(
                    [=]{
                        while(!done){
                            std::unique_lock<std::mutex> lk(mut);
                            cond.wait(lk, [=]{return !taskQueue.empty();});
                            auto task = taskQueue.front();
                            taskQueue.pop();
                            lk.unlock();
                            task();
                        }
                    }
                );
            threads.push_back(std::move(t));
        }
    }

    void join(){
        for(auto& t: threads){
            t.join();
        }
    }

    void commitTask(std::function<void()> task){
        {
            std::lock_guard<std::mutex> lk(mut);
            taskQueue.push(task);
        }
        cond.notify_all();
    }

};

SingletonThreadPool* SingletonThreadPool::_tp = nullptr;

#endif
