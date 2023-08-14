#ifndef FUNC_H
#define FUNC_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>

// #define connfdET    //边缘触发非阻塞
// #define listenfdET 

#define connfdLT    //水平触发阻塞
#define listenfdLT 

int setfd_noblocking(int fd);
void addfd(int epollfd, int socketfd, bool one_shot);
void addsignal(int signum, void(handler)(int));

#endif