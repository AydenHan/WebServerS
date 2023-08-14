#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <list>
#include <vector>
#include <atomic>
#include <chrono>
#include <exception>
#include <pthread.h>

#include "lock/lock.h"

template<typename T>
class ThreadPool 
{
private:
    static void* transfer(void* arg);       // 工作转发函数
    void work();                            // 具体工作函数
public:
    ThreadPool(int thread_num = 8, int max_requests = 10000, bool dynamic_ctl = false);
    ~ThreadPool();

    bool append_task(T* req);

    void set_dynamic_ctl(bool val);
    bool append_thread(int new_num);

    // 监控线程池状态
    int get_thread_num() const { return _thread_num; }
    int get_task_num() const { return _taskqueue.size(); }
    int get_completed_task_num() const { return _completed_tasknum.load(); }
    double get_run_time() const { 
        std::chrono::duration<double> diff = std::chrono::steady_clock::now() - start_time; 
        return diff.count();
    }
private:
    std::vector<pthread_t> _threads;    // 存储线程的数组，大小为 _thread_num
    int _thread_num;                    // 线程池中的线程数
    std::atomic<bool> _run;             // 原子布尔变量，标记线程终止运行

    int _max_requests;                  // 请求队列中允许的最大请求数
    std::list<T*> _taskqueue;           // 请求任务队列

    MutexS _queue_mutex;                // 保护请求队列的互斥锁
    SemS _queue_sem;                    // 通知线程的信号量

    bool _dynamic_ctl = false;          // 是否允许动态增减线程数量
    std::atomic<int> _completed_tasknum = 0;             // 已完成的任务数
    std::chrono::steady_clock::time_point start_time;    // 记录线程池开始运行的时间
};

template<typename T>
ThreadPool<T>::ThreadPool(int thread_num, int max_requests, bool dynamic_ctl) 
    : _thread_num(thread_num), _max_requests(max_requests), _dynamic_ctl(dynamic_ctl), _run(true) {

    if(thread_num <= 0 || max_requests <= 0)
        throw "Error. Parameter out of range!";
    _threads.resize(thread_num);

    for(int i = 0; i < thread_num; ++i) {
        if(pthread_create(&_threads[i], NULL, transfer, (void*)this)) {
            throw "Error. Create thread failed!";
        }
        if(pthread_detach(_threads[i])) {
            throw "Error. Thread unjoinable failed!";
        }
    }
    start_time = std::chrono::steady_clock::now();
}

template<typename T>
ThreadPool<T>::~ThreadPool() {
    _run = false;
}

template <typename T>
bool ThreadPool<T>::append_task(T* req) {
    _queue_mutex.lock();
    if (_taskqueue.size() >= _max_requests) {
        _queue_mutex.unlock();
        return false;
    }
    _taskqueue.emplace_back(req);
    _queue_mutex.unlock();
    _queue_sem.post();
    return true;
}

template <typename T>
void* ThreadPool<T>::transfer(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    pool->work();
    return pool;
}
template <typename T>
void ThreadPool<T>::work() {
    while(_run) {
        _queue_sem.wait();
        _queue_mutex.lock();
        if(_taskqueue.empty()) {
            _queue_mutex.unlock();
            continue;
        }
        T* request = _taskqueue.front();
        _taskqueue.pop_front();
        _queue_mutex.unlock();
        if(!request)
            continue;
            
        // handle

        ++_completed_tasknum;
    }
}

template <typename T>
void ThreadPool<T>::set_dynamic_ctl(bool val) {
    _dynamic_ctl = val;
}

template <typename T>
bool ThreadPool<T>::append_thread(int new_num) {
    if(!_dynamic_ctl)   return false;
    _threads.resize(_thread_num + new_num);
    for(int i = 0; i < new_num; ++i) {
        if(pthread_create(&_threads[_thread_num + i], NULL, transfer, (void*)this)) {
            return false;
        }
        if(pthread_detach(_threads[_thread_num + i])) {
            return false;
        }
    }
    _thread_num += new_num;
    return true;
}



#endif