#ifndef TIMER
#define TIMER

#include <functional>
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
    //void (*callback)(cli_data*);
    std::function<void(cli_data*)> callback;
};

class Timer
{
public:
    Timer() : head(nullptr), tail(nullptr) {}
    ~Timer() {
        TimerNode* dummy = head;
        while (dummy) {
            head = dummy->next;
            delete dummy;
            dummy = head;
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
        TimerNode* dummy = timer->next;
        if(!dummy || timer->overdue <= dummy->overdue)
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
    /**
     * @description: 当连接超时时，更新链表 TODO
     */    
    void timeout() {
        if(!head)   return;
        time_t cur = time(NULL);
        TimerNode* dummy = head;
        while(dummy) {
            if(cur < dummy->overdue)
                break;
            dummy->callback(dummy->user_data);
            head = dummy->next;
            if(head)    head->prev = nullptr;
            delete dummy;
            dummy = head->next;
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
        TimerNode* dummy = pre->next;
        while(dummy) {
            if(timer->overdue < dummy->overdue) {
                pre->next = timer;
                timer->next = dummy;
                dummy->prev = timer;
                timer->prev = pre;
                return;
            }
            pre = dummy;
            dummy = dummy->next;
        }
        if(!dummy) {
            pre->next = timer;
            timer->prev = pre;
            timer->next = nullptr;
            tail = timer;
        }
    }
};


#endif