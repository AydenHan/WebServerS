#ifndef LOCK_H
#define LOCK_H

#include <pthread.h>
#include <semaphore.h>

class ConditionS
{
private:
    pthread_cond_t _condt;
public:
    ConditionS() { _condt = PTHREAD_COND_INITIALIZER; }
    ~ConditionS() { pthread_cond_destroy(&_condt); }

    bool wait(pthread_mutex_t* mutex) {
        return pthread_cond_wait(&_condt, mutex) == 0;
    }
    bool timewait(pthread_mutex_t* mutex, const struct timespec t) {
        return pthread_cond_timedwait(&_condt, mutex, &t) == 0;
    }
    bool signal() {
        return pthread_cond_signal(&_condt) == 0;
    }
    bool broadcast() {
        return pthread_cond_broadcast(&_condt) == 0;
    }
};

class SemS
{
public:
    SemS() { sem_init(&_sem, 0, 0); }
    SemS(int val) { sem_init(&_sem, 0, val); }
    ~SemS() { sem_destroy(&_sem); }

    // 等待信号量大于0后结束阻塞
    bool wait() { return sem_wait(&_sem) == 0; }            
    // 信号量 ≤ 0 直接返回
    bool try_wait() { return sem_trywait(&_sem) == 0; }     
    // 信号量加1
    bool post() { return sem_post(&_sem) == 0; }            
private:
    sem_t _sem;
};

class MutexS
{
public:
    MutexS() { _mutex = PTHREAD_MUTEX_INITIALIZER; }
    ~MutexS() { pthread_mutex_destroy(&_mutex); }

    void lock() { pthread_mutex_lock(&_mutex); }

    bool try_lock() { return pthread_mutex_trylock(&_mutex) == 0; }

    void unlock() { pthread_mutex_unlock(&_mutex); }

    pthread_mutex_t* get_mutex() { return &_mutex; }
private:
    pthread_mutex_t _mutex;
};

#endif