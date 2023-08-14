#ifndef TIMER
#define TIMER

#include <time.h>
#include <netinet/in.h>

class TimerNode;

struct cli_data
{
    sockaddr_in addr;
    int sockfd;
    TimerNode* timer;
};

class TimerNode
{
public:
    TimerNode() : prev(nullptr), next(nullptr) {}
public:
    time_t  overdue;
    TimerNode* prev;
    TimerNode* next;
    cli_data* user_data;
    void (*callback)(cli_data*);
};

class Timer
{
public:
    Timer() : head(nullptr), tail(nullptr) {}
    ~Timer() {
        TimerNode* tmp = head;
        while (tmp) {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }
private:
    TimerNode* head;
    TimerNode* tail;
public:
    /**
     * @description: 将定时器 timer 插入到升序链表中
     * @param {TimerNode*} timer 待插入的定时器
     */         
    void add_timer(TimerNode* timer) {
        if(!timer)  return;
        if(!head) {
            head = tail = timer;
            return;
        }
        if(timer->overdue < head->overdue) {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        add_timer_from(timer, head);
    }
    /**
     * @description: 将定时器 timer 从升序链表中删除
     * @param {TimerNode*} timer 待删除的定时器
     */    
    void del_timer(TimerNode* timer) {
        if(!timer)  return;
        if((timer == head) && (timer == tail)) {
            delete timer;
            head = tail = nullptr;
            return;
        }
        if(timer == head) {
            head = head->next;
            head->prev = nullptr;
            delete timer;
            return;
        }
        if(timer == tail) {
            tail = tail->prev;
            tail->next = nullptr;
            delete timer;
            return;
        }
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }
    /**
     * @description: 将定时器 timer 调整至升序链表中的正确位置
     * @param {TimerNode*} timer 待更新位置的定时器
     */    
    void adjust_timer(TimerNode* timer) {
        if(!timer)  return;
        TimerNode* tmp = timer->next;
        if(!tmp || timer->overdue <= tmp->overdue)
            return;
        if(timer == head) {
            head = head->next;
            head->prev = nullptr;
            add_timer_from(timer, head);
        }
        else {
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            add_timer_from(timer, timer->next);
        }
    }
private:
    /**
     * @description: 将定时器 timer 插入到升序链表中（从 start 节点开始遍历）
     * @param {TimerNode*} timer 待插入的定时器
     * @param {TimerNode*} start 查询插入位置时的起始节点
     */    
    void add_timer_from(TimerNode* timer, TimerNode* start) {
        TimerNode* pre = start;
        TimerNode* tmp = pre->next;
        while(tmp) {
            if(timer->overdue < tmp->overdue) {
                pre->next = timer;
                timer->next = tmp;
                tmp->prev = timer;
                timer->prev = pre;
                return;
            }
            pre = tmp;
            tmp = tmp->next;
        }
        if(!tmp) {
            pre->next = timer;
            timer->prev = pre;
            timer->next = nullptr;
            tail = timer;
        }
    }
};


#endif