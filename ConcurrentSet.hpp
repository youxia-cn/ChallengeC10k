#ifndef CONCURRENT_SET_HPP
#define CONCURRENT_SET_HPP

#include <set>
#include <mutex>

template <typename T>
class ConcurrentSet{
    std::set<T> contents;
    std::mutex mut;
public:
    size_t size(){
        std::lock_guard<std::mutex> lk(mut);
        return contents.size();
    }

    void insert(T& content){
        std::lock_guard<std::mutex> lk(mut);
        contents.insert(content);
    }

    void erase(T& content){
        std::lock_guard<std::mutex> lk(mut);
        contents.erase(content);
    }
};

#endif
