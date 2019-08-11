// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "EventLoopThread.h"
#include <functional>


EventLoopThread::EventLoopThread()
:   loop_(NULL),
    exiting_(false),
    thread_(bind(&EventLoopThread::threadFunc, this), "EventLoopThread"), // 初始化线程对象
    mutex_(),
    cond_(mutex_) // 初始化条件变量
{ }

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != NULL)
    {
        loop_->quit();
        thread_.join();
    }
}

// 开启线程，内部通过条件变量，保证线程内事件循环类的loop开启
// 因为是通过一个EventLoopThread对象创建所有子线程，所以要用条件变量 + 互斥锁
EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
	// 创建线程
    thread_.start();

	// 条件变量使用原则
	// 1. 加锁（在已上锁的情况下才可以调用wait()）
	// 2. 把判断条件和wait()放到while循环中
    {
        MutexLockGuard lock(mutex_);
        // 一直等到threadFun在Thread里真正跑起来
        while (loop_ == NULL)
            cond_.wait();
    }
    return loop_;
}

// EventLoopThread运行的线程函数
// 栈上变量EventLoop对象
void EventLoopThread::threadFunc()
{
    EventLoop loop;

    {
        MutexLockGuard lock(mutex_); // 加锁
        loop_ = &loop; 
        cond_.notify(); // 条件变量通知
    }

    loop.loop(); // 调用loop()开启事件循环
    //assert(exiting_);
    loop_ = NULL;
}