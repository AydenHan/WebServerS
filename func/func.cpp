#include "func.h"


/**
 * @description:    将文件描述符设置为非阻塞
 * @param {int} fd  待设置的文件描述符
 * @return {*}      文件描述符修改前的标志
 */
int setfd_noblocking(int fd) {
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

/**
 * @description: 在内核事件表中注册读事件，ET模式
 * @param {int} epollfd     epoll句柄
 * @param {int} socketfd    待监听的文字描述发
 * @param {bool} one_shot   是否开启EPOLLONESHOT
 */
void addfd(int epollfd, int socketfd, bool one_shot) {
    epoll_event event;
    event.data.fd = socketfd;

#ifdef connfdET
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
#endif

#ifdef connfdLT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif

#ifdef listenfdET
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
#endif

#ifdef listenfdLT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif

    if(one_shot)
        event.events = EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event);

    setfd_noblocking(socketfd);
}

/**
 * @description: 检查或修改指定信号的设置
 * @param {int} signum          要操作的信号
 * @param {function} handler    信号处理函数
 */
void addsignal(int signum, void(handler)(int)) {
    struct sigaction sigact;
    sigact.sa_handler = handler;
    // 在信号处理时，临时屏蔽所有信号
    sigfillset(&sigact.sa_mask);
    if(sigaction(signum, &sigact, nullptr) == -1)
        throw "Signal setting failed!";
}

