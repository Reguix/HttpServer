// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "noncopyable.h"
#include <pthread.h>
#include <cstdio>

class MutexLock: noncopyable
{
public:
    MutexLock()
    {
        pthread_mutex_init(&mutex, NULL);
    }
    ~MutexLock()
    {
        pthread_mutex_lock(&mutex);
        pthread_mutex_destroy(&mutex);
    }
    void lock()
    {
        pthread_mutex_lock(&mutex);
    }
    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }
    pthread_mutex_t *get()
    {
        return &mutex;
    }
private:
    pthread_mutex_t mutex;

// 友元类不受访问权限影响
private:
    friend class Condition;
};

// 不手工调用lock()和unlock()函数，一切交给栈上的Guard对象的构造函数和析构函数负责
// 封装加锁和解锁，作用域等于临界区域，加锁和解锁在同一个进程
class MutexLockGuard: noncopyable
{
public:
    explicit MutexLockGuard(MutexLock &_mutex):
    mutex(_mutex)
    {
        mutex.lock();
    }
    ~MutexLockGuard()
    {
        mutex.unlock();
    }
private:
    MutexLock &mutex;
};