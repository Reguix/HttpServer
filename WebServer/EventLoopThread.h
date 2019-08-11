// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "base/Condition.h"
#include "base/MutexLock.h"
#include "base/Thread.h"
#include "base/noncopyable.h"
#include "EventLoop.h"

/* 
      EventLoopThread类
      one loop per thread原则
      EventLoopThread类会启动自己的线程startLoop()，并在其中运行EventLoop::loop()
*/

class EventLoopThread :noncopyable
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();
    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
};