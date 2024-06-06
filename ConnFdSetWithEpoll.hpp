#ifndef CONN_FD_SET_HPP
#define CONN_FD_SET_HPP

#include <set>
#include <sys/epoll.h>
#include <mutex>
#include <condition_variable>
#include <iostream>

class ConnFdSetWithEpoll{
    std::set<int> fds;
    int epfd;
    std::mutex mut;
    std::condition_variable cond;

public:
    ConnFdSetWithEpoll(int size){
        epfd = epoll_create(size);
    }
    
    size_t size(){
        std::lock_guard<std::mutex> lk(mut);
        return fds.size();
    }

    void insert(int connfd){
        std::lock_guard<std::mutex> lk(mut);
        fds.insert(connfd);
        
        struct epoll_event temp_epoll_event;
        temp_epoll_event.events = EPOLLIN;
        temp_epoll_event.data.fd = connfd;
        if( epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &temp_epoll_event) == -1 ){
            perror("epoll_ctl add");
        }else{
            std::cout << "Add connect socket " << connfd << " sucessful!" << std::endl;
            std::cout << "There are " << fds.size() << " sockets in epoll." << std::endl;
        }
    }
    
    void erase(int connfd){
        std::lock_guard<std::mutex> lk(mut);
        fds.erase(connfd);
        if( epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL) == -1 ){
            perror("epoll_ctl add");
        }else{
            std::cout << "Delete connect socket " << connfd << " sucessful!" << std::endl;
            std::cout << "There are " << fds.size() << " sockets in epoll." << std::endl;
        }
    }

    int epfd(){
        return epfd;
    }

};

#endif
