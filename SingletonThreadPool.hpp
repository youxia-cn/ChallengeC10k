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
    static SingletonThreadPool* pThreadPool;
    static int nThreadAmount;

    std::queue<std::function<void()>> taskQueue;
    std::mutex mut;
    std::condition_variable cond;
    std::vector<std::thread> threads;
    bool done=false;

    public:
    static SingletonThreadPool& getThreadPool(){
        if(pThreadPool == nullptr){
            pThreadPool = new SingletonThreadPool(nThreadAmount);
        }
        return *pThreadPool;
    }

    SingletonThreadPool(int nThreadAmount){
        for(int i=0; i<nThreadAmount; i++){
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

    void terminate(){
        done = true;
    }

    static void initThreadAmount(int n){
        if(pThreadPool == nullptr){
            nThreadAmount = n;
        }else{
            std::cerr << "ThreadPool has been created, cann't changed the thread amount!" << std::endl;
        }
    }

};

SingletonThreadPool* SingletonThreadPool::pThreadPool = nullptr;
int SingletonThreadPool::nThreadAmount = 20;

#endif
