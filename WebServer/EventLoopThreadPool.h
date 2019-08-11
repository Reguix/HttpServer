// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "base/noncopyable.h"
#include "EventLoopThread.h"
#include "base/Logging.h"
#include <memory>
#include <vector>
/*
    线程池类
    one loop per thread思想
    就是TcpServer自己的Eventloop（baseLoop）只用来接受新连接
    新连接会从event loop pool 中选择一个EventLoop来用
*/

class EventLoopThreadPool : noncopyable
{
public:
    EventLoopThreadPool(EventLoop* baseLoop, int numThreads);

    ~EventLoopThreadPool()
    {
        LOG << "~EventLoopThreadPool()";
    }
    void start();

    EventLoop *getNextLoop();

private:
	/*
        线程池拥有一个EventLoop对象
        和线程对象vector
        和EventLoop对象vector
	*/
    EventLoop* baseLoop_;
    bool started_;
    int numThreads_;
    int next_; // 下一个要
    std::vector<std::shared_ptr<EventLoopThread>> threads_; // 池中所有线程对象的指针
    std::vector<EventLoop*> loops_; // 池中所有线程对象中
};