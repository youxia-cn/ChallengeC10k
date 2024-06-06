#include "SingletonThreadPool.hpp"
#include "ConnFdSetWithEpoll.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <algorithm>
#include <iostream>

#define MAX_THREADS 20
#define MAX_SOCKETS 10000
#define SERVER_PORT 8888
#define MAX_READS 1024

class ServerSocketTask{
    int sockfd;
    ConnFdSetWithEpoll& connfds;
    SingletonThreadPool& threadPool;
public:
    ServerSocketTask(int fd, ConnFdSetWithEpoll& cfds, SingletonThreadPool& tp):sockfd(fd), connfds(cfds), threadPool(tp){
    }

    void operator()(){
        int buf[MAX_READS/sizeof(int)];
        bzero(buf, MAX_READS);
        int n = read(sockfd, buf, MAX_READS);
        if(n==0){
            connfds.erase(sockfd);
        }else{
//            std::cout << "Socket " << sockfd << " received client's data: " << buf[0] << " " << buf[1] 
  //              << " ... " << buf[n/sizeof(int)-2] << " " << buf[n/sizeof(int)-1] << std::endl;
//            std::for_each(buf, buf+n/sizeof(int)-1, [](int& n){ n = abs(n); } );
  //          std::sort(buf, buf+n/sizeof(int)-1);
            write(sockfd, buf, n);
    //        std::cout << "Socket " << sockfd << " sent data: " << buf[0] << " " << buf[1] 
      //          << " ... " << buf[n/sizeof(int)-2] << " " << buf[n/sizeof(int)-1] << std::endl;
        }
    }
};

int main(){

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    if( bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1 ){
        perror("bind");
    }else{
        std::cout << "Bind Successful!" << std::endl;
    }

    if( listen(listenfd, 20) == -1 ){
        perror("listen");
    }else{
        std::cout << "Listen Successful!" << std::endl;
    }

    ConnFdSetWithEpoll connfds(MAX_SOCKETS);
    //创建一个新线程，在新线程中调用 epoll_wait，并将有响应的 socket 添加到 ThreadPool 的任务队列
    SingletonThreadPool& threadPool = SingletonThreadPool::getThreadPool();
    std::thread t(
            [&connfds, &threadPool]{
                struct epoll_event ep[MAX_SOCKETS];
                int nready;
                while(true){
                    nready = epoll_wait(connfds.getEpfd(), ep, MAX_SOCKETS, -1);
                    if(nready == -1){
                        perror("epoll_wait");
                    }else{
                        for(int i=0; i<nready; i++){
                            if(!(ep[i].events & EPOLLIN))
                                continue;
                            int sockfd = ep[i].data.fd;
                            threadPool.commitTask( ServerSocketTask(sockfd, connfds, threadPool) );
                        }
                    }
                }
            }
        );

    //在本线程中反复 accept
    while(true){
        if(connfds.size() < MAX_SOCKETS){
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
            char str[INET_ADDRSTRLEN];
            std::cout << "Received from " << inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)) << ":" << ntohs(client_addr.sin_port) << std::endl;
            connfds.insert(connfd);
        }else{
            std::this_thread::yield();
        }
    }

    t.join();
    threadPool.join();

    return 0;

}
