#include "ThreadPool.hpp"
#include <arpa/inet.h>
#include <mutex>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <random>
#include <set>

#define MAX_SOCKETS 10000
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 8888
#define MAX_READS 1024
#define MAX_THREADS 20

class ConcurrentFdSet{
    std::set<int> fds;
    std::mutex mut;
    public:
    size_t size(){
        std::lock_guard<std::mutex> lk(mut);
        return fds.size();
    }

    void insert(int fd){
        std::lock_guard<std::mutex> lk(mut);
        fds.insert(fd);
    }

    void erase(int fd){
        std::lock_guard<std::mutex> lk(mut);
        fds.erase(fd);
    }
};

class ClientSocketTask{
    int sockfd;
    ConcurrentFdSet& fds;
    ThreadPool& threadPool;
    int* randomNums;
    std::random_device& rd;

public:
    ClientSocketTask(int fd, ConcurrentFdSet& fds, ThreadPool& tp, int* rdn, std::random_device& rde):
        sockfd(fd), fds(fds), threadPool(tp), randomNums(rdn), rd(rde){
        }

    void operator()(){
        int n = rd() % 10;

        for(int i=0; i<n; i++){
            int start = rd()%(100000 - MAX_READS/sizeof(int));
            std::cout << "thread_id:" << std::this_thread::get_id() << " ,fd:" << sockfd << " ,n:" << n << " ,i:" << i << std::endl;
            send(sockfd, randomNums+start, MAX_READS, 0);
            std::cout << "Send data: " << randomNums[start] << " " << randomNums[start+1] << " ... "
                << randomNums[start+MAX_READS/sizeof(int)-1] << " " << randomNums[start+MAX_READS/sizeof(int)-1] << std::endl;
            int receivedNums[MAX_READS/sizeof(int)];
            recv(sockfd, receivedNums, MAX_READS, 0);
            std::cout << "Receiv data: " << receivedNums[0] << " " << receivedNums[1] << " ... "
                << receivedNums[MAX_READS/sizeof(int)-1] << " " << receivedNums[MAX_READS/sizeof(int)-1] << std::endl;
        }

        //五分之一的概率连接关闭，五分之四的概率提交新任务
        if(n%5 == 0){
            fds.erase(sockfd);
            close(sockfd);
            std::cout << "Close socket, fd:" << sockfd << std::endl;
        }else{
            std::cout << "Reuse socket and add task, sockfd:" << sockfd << std::endl;
            threadPool.commitTask( *this );
        }
    }
};

int main(){

    //先准备 100000 个整数
    std::random_device rd;
    int randomNums[100000];
    for(int i=0; i<100000; i++){
        randomNums[i] = rd();
    }

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr);
    server_addr.sin_port = htons(SERVER_PORT);

    ConcurrentFdSet fds;
    ThreadPool threadPool(MAX_THREADS);

    while(true){
        if(fds.size() < MAX_SOCKETS){
            int sockfd = socket(PF_INET, SOCK_STREAM, 0);
            if(sockfd == -1){
                perror("create socket");
                return 1;
            }
            if(connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
                perror("connect");
                close(sockfd);
            }else{
                fds.insert(sockfd);
                threadPool.commitTask( ClientSocketTask(sockfd, fds, threadPool, randomNums, rd) );
                std::cout << "Add task, sockfd:" << sockfd << std::endl;
            }
        }else{
            std::this_thread::yield();
        }
    }

    threadPool.join();

    return 0;
}

