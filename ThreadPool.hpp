#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <iostream>

class ThreadPool{

    std::queue<std::function<void()>> taskQueue;
    std::mutex mut;
    std::condition_variable cond;
    std::vector<std::thread> threads;
    bool done=false;

    public:

    ThreadPool(int nThreadAmount){
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

};

#endif
