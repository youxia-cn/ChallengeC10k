#include "ConcurrentFdSetWithEpoll.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <numeric>
#include <stdlib.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <vector>

#define MAX_THREADS 20
#define MAX_SOCKETS 10000
#define SERVER_PORT 8888
#define MAX_READS 8192

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

    ConcurrentFdSetWithEpoll* pfds = static_cast<ConcurrentFdSetWithEpoll*> (operator new(MAX_THREADS * sizeof(ConcurrentFdSetWithEpoll)));
    for(int i=0; i<MAX_THREADS; i++){
        new(pfds+i)ConcurrentFdSetWithEpoll(MAX_SOCKETS);
    }
    int fds_count[MAX_THREADS];

    //在循环调用 accept 之前，先创建线程，在线程里面执行 epoll_wait
    std::vector<std::thread> threads;
    for(int i=0; i<MAX_THREADS; i++){
        std::thread t(
                [&pfds](ConcurrentFdSetWithEpoll& fds){
                    struct epoll_event ep[MAX_SOCKETS];
                    int nready;
                    while(true){
                        nready = epoll_wait(fds.getEpfd(), ep, MAX_SOCKETS, 10);
                        if(nready == -1){
                            perror("epoll_wait");
                        }else{
                            for(int i=0; i<nready; i++){
                                if(!(ep[i].events & EPOLLIN)){
                                    continue;
                                }else{
                                    int sockfd = ep[i].data.fd;
                                    int buf[MAX_READS/sizeof(int)];
                                    bzero(buf, MAX_READS);
                                    int nBytes = read(sockfd, buf, MAX_READS);
                                    int nNums = nBytes/sizeof(int);
                                    if(nBytes==0){
                                        fds.erase(sockfd);
                                        int fds_count[MAX_THREADS];
                                        for(int i=0; i<MAX_THREADS; i++){
                                            fds_count[i] = pfds[i].size();
                                        }
                                        int sockets_count = std::accumulate(&fds_count[0], &fds_count[MAX_THREADS-1], 0);
                                        std::cout << "There are " << sockets_count << " sockets in epoll!" << std::endl;
                                    }else{
                                        std::cout << "Socket " << sockfd << " received client's data: " << buf[0] << " " << buf[1] 
                                            << " ... " << buf[nNums-2] << " " << buf[nNums-1] << std::endl;
                                        std::for_each(&buf[0], &buf[nNums-1], [](int& n){ n = abs(n); } );
                                        std::sort(&buf[0], &buf[nNums-1]);
                                        write(sockfd, buf, nBytes);
                                        std::cout << "Socket " << sockfd << " sent data: " << buf[0] << " " << buf[1] 
                                            << " ... " << buf[nNums-2] << " " << buf[nNums-1] << std::endl;
                                    }
                                }
                            }
                        }
                    }
                }, std::ref(pfds[i]));
        threads.push_back(std::move(t)); 
    }

    //在本线程中反复 accept
    while(true){
        //先找出 fds 中 socket 数量最少的，并计算所有的 fds 中的 socket 总数
        for(int i=0; i<MAX_THREADS; i++){
            fds_count[i] = pfds[i].size();
        }
        int sockets_count = std::accumulate(&fds_count[0], &fds_count[MAX_THREADS-1], 0);
        auto min_fds = std::min_element(&fds_count[0], &fds_count[MAX_THREADS-1]);
        if(sockets_count < MAX_SOCKETS){
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
            char str[INET_ADDRSTRLEN];
            std::cout << "Received from " << inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)) << ":" << ntohs(client_addr.sin_port) << std::endl;
            pfds[min_fds-fds_count].insert(connfd);
            std::cout << "There are " << sockets_count + 1 << " sockets in epoll!" << std::endl;
        }else{
            std::this_thread::yield();
        }
    }

    for(auto& t:threads){
        t.join();
    }

    return 0;

}
