// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "base/Thread.h"
#include "Epoll.h"
#include "base/Logging.h"
#include "Channel.h"
#include "base/CurrentThread.h"
#include "Util.h"
#include <vector>
#include <memory>
#include <functional>

#include <iostream>
using namespace std;

/*    
      事件循环类
      EventLoop拥有并管理一个Epoll类对象，并管理归属于本EventLoop对象的Channel对象
      （每个Channel对象自始至终只属于一个EventLoop）
      创建了EventLoop对象的线程是IO线程，其主要功能是运行事件循环loop()，
      不断的获取就绪事件Epoll:poll()，并处理事件Channel::handleEvent()

      EventLoop实现了一个功能：在IO线程中执行某个用户回调，即runInLoop()函数；
      IO线程平时阻塞在loop()的epoll_wait()调用中，为了立刻执行用户回调，需要唤醒线程，
      这里利用eventfd来唤醒线程

*/
class EventLoop
{
public:
    typedef std::function<void()> Functor;
    EventLoop();
    ~EventLoop();
    void loop(); // 循环函数
    void quit(); // 退出函数
    void runInLoop(Functor&& cb); // 在IO线程内执行某个用户任务回调
    void queueInLoop(Functor&& cb); // cb会被加入队列中，IO线程会被唤醒来调用这个cb
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); } // 判断是否是当前线程，每个线程只能有一个EventLoop对象
    void assertInLoopThread() // 检查当前线程是否已经创建了其他EventLoop对象
    {
        assert(isInLoopThread());
    }

	// 关闭一个fd的读写
    void shutdown(shared_ptr<Channel> channel)
    {
        shutDownWR(channel->getFd());
    }

	// 从IO复用类对象中poller_中移除Channel对象
    void removeFromPoller(shared_ptr<Channel> channel)
    {
        //shutDownWR(channel->getFd());
        poller_->epoll_del(channel);
    }

	// 向IO复用类对象中poller_中更新Channel对象
    void updatePoller(shared_ptr<Channel> channel, int timeout = 0)
    {
        poller_->epoll_mod(channel, timeout);
    }

	// 向IO复用类对象中poller_中添加Channel对象
    void addToPoller(shared_ptr<Channel> channel, int timeout = 0)
    {
        poller_->epoll_add(channel, timeout);
    }
    
private:
    // 声明顺序 wakeupFd_ > pwakeupChannel_
    bool looping_;
    shared_ptr<Epoll> poller_; // 每个事件循环中都有一个IO复用
    int wakeupFd_; // eventfd
    bool quit_;
    bool eventHandling_;
    mutable MutexLock mutex_;
    std::vector<Functor> pendingFunctors_; // 保存回调函数，暴露给了其他线程，需要用mutex保护
    bool callingPendingFunctors_; // 是否正在调用pending functor
    const pid_t threadId_; // 线程id，EventLoop的构造函数会记住本对象所属的线程
    shared_ptr<Channel> pwakeupChannel_; // 把wakeupFd_包装进Channel对象来管理
    
    void wakeup();
    void handleRead();
    void doPendingFunctors();
    void handleConn();
};
