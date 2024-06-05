#include "SingletonThreadPool.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>

#define MAX_SOCKET 10000
#define SERVER_PORT 8888
#define MAX_READ 4096

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

    ConnFdSet connfds;
    while(true){
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
        char str[INET_ADDRSTRLEN];
        std::cout << "Received from " << inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)) << ":" << ntohs(client_addr.sin_port) << std::endl;
        connfds.insert(connfd);
    }

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

