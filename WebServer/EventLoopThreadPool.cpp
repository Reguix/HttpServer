// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "EventLoopThreadPool.h"

// 构造函数，baseLoop以调用构造函数者所拥有的EventLoop传入
EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, int numThreads)
:   baseLoop_(baseLoop),
    started_(false),
    numThreads_(numThreads),
    next_(0)
{
    if (numThreads_ <= 0)
    {
        LOG << "numThreads_ <= 0";
        abort();
    }
}
// 
void EventLoopThreadPool::start()
{
    baseLoop_->assertInLoopThread();
    started_ = true;
    for (int i = 0; i < numThreads_; ++i)
    {
        // 创建指定数目的线程
        std::shared_ptr<EventLoopThread> t(new EventLoopThread());
        threads_.push_back(t);
        loops_.push_back(t->startLoop()); // 开启子线程的loop() 
    }
}

// round-robin轮询调度
EventLoop *EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    assert(started_);
    EventLoop *loop = baseLoop_;
	// 如果子线程为空，就返回主线程loop
    if (!loops_.empty())
    {
        loop = loops_[next_];
        next_ = (next_ + 1) % numThreads_; // 不为空则轮询
    }
    return loop;
}