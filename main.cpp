#include <iostream>
#include <stdexcept>
#include <array>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>

#include "lock/lock.h"
#include "threadpool/threadpool.h"
#include "http/http_conn.h"
#include "timer/timer.h"

#define MAX_FD 65536            // 最大文件描述符
#define MAX_EVENT 10000     // 最大事件数
#define TIMESLOT 5              // 最小超时单位

static int epollfd = 0;         // 全局epoll实例
static int pipefd[2];           // 全局匿名管道，0用于读取，1用于写入
static Timer timerList;         // 定时器链表

/**
 * @description: 信号处理函数
 * @param {int} arg 触发该函数的信号
 */
void signal_handler(int arg) {
    int cur_errno = errno;      // 保证该函数的可重入性
    int msg = arg;
    send(pipefd[1], (char*)msg, 1, 0);
    errno = cur_errno;
}

/**
 * @description: 定时器回调函数，删除在socket上非活动连接的注册事件，并关闭
 * @param {cli_data*} data 定时器所属的用户数据
 */
void timer_handler(cli_data* data) {
    if(!data)   return;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, data->sockfd, nullptr);
    close(data->sockfd);
    //
}


int main(int argc, char *argv[])
{
    // args
    int port = atoi(argv[1]);

    // Socket
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);     // 创建一个监听socket描述字
    if(listenfd < 0)
        throw "Invalid Socket fd!";

    sockaddr_in address;                                // 设置ipv4协议、任意ip、固定端口
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    int reuse_addr_ctl = 1;                             // 允许端口复用
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr_ctl, sizeof(reuse_addr_ctl));

    // 把用于通信的地址和端口绑定到 socket上
    if(bind(listenfd, (sockaddr*)&address, sizeof(address)) < 0)
        throw "Bind failed!";
    
    // 等待客户的连接请求
    if(listen(listenfd, 5) < 0)
        throw "Set Listen failed!";

    // epoll
    epoll_event events[MAX_EVENT];
    epollfd = epoll_create(5);              // 创建一个epoll实例
    if(epollfd == -1) {
        throw "Epoll create failed!";
    }
    addfd(epollfd, listenfd, false);        // 在内核事件表中注册监听事件

    // signal
    addsignal(SIGPIPE, SIG_IGN);            // TCP服务端close后继续收到两次数据,屏蔽SIGPIPE信号
    addsignal(SIGALRM, signal_handler);     // 超时处理
    addsignal(SIGTERM, signal_handler);     // 进程被kill时处理

    // pipe
    if(socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd) == -1) {     // 创建全双工的匿名管道，用于通信
        throw "Create pipe failed!";
    }
    setfd_noblocking(pipefd[1]);            // 设置为非阻塞
    addfd(epollfd, pipefd[0], false);       // 在内核事件表中注册该管道的一端

    // timer
    std::array<cli_data, MAX_FD> cli_timers;

    bool stop_server = false;
    bool timeout = false;
    alarm(TIMESLOT);                        // 设置超时闹钟，并开始运行服务器
    while(!stop_server) 
    {
        int nfds = epoll_wait(epollfd, events, MAX_EVENT, -1);
        for(int i = 0; i < nfds; ++i) 
        {
            int cur_fd = events[i].data.fd;
            // 产生一个新的连接
            if(cur_fd == listenfd)
            {
                struct sockaddr_in cli_addr;
                socklen_t cli_addr_len = sizeof(cli_addr);
#ifdef listenfdLT
                int connfd = accept(listenfd, (sockaddr*)&cli_addr, &cli_addr_len);
                if(connfd < 0)
                    throw "Accept Error!";
                
                // 存储客户端信息
                cli_timers[connfd].addr = cli_addr;
                cli_timers[connfd].sockfd = connfd;
                // 创建定时器
                TimerNode* timer = new TimerNode();
                timer->user_data = &cli_timers[connfd];
                timer->callback = timer_handler;
                time_t cur_time = time(NULL);
                timer->overdue = cur_time + 3 * TIMESLOT;
                cli_timers[connfd].timer = timer;
                // 存储定时器
                timerList.add_timer(timer);
#endif
#ifdef listenfdET
                printf("1");
#endif
            }
            // 服务器端关闭连接，移除对应的定时器
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                TimerNode* timer = cli_timers[cur_fd].timer;
                if(timer) {
                    timer->callback(&cli_timers[cur_fd]);
                    timerList.del_timer(timer);
                }
            }
            // 处理信号
            else if(cur_fd == pipefd[0] && (events[i].events & EPOLLIN))
            {
                
            }
            // 接收到数据，读socket
            else if(events[i].events & EPOLLIN)
            {

            }
            // 有数据待发送，写socket
            else if(events[i].events & EPOLLOUT)
            {
                
            }
        }
    }
    close(epollfd);
    close(listenfd);
    close(pipefd[1]);
    close(pipefd[0]);
    return 0;
}