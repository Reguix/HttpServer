// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "EventLoop.h"
#include "base/Logging.h"
#include "Util.h"
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <iostream>
using namespace std;

__thread EventLoop* t_loopInThisThread = 0;

// 创建eventfd并返回
int createEventfd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

EventLoop::EventLoop()
:   looping_(false),
    poller_(new Epoll()),
    wakeupFd_(createEventfd()),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    pwakeupChannel_(new Channel(this, wakeupFd_)) // 用pwakeupChannel_来管理wakeupFd_
{
    if (t_loopInThisThread)
    {
        //LOG << "Another EventLoop " << t_loopInThisThread << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    //pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    // 设置pwakeupChannel_的IO回调事件
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
    pwakeupChannel_->setReadHandler(bind(&EventLoop::handleRead, this));
    pwakeupChannel_->setConnHandler(bind(&EventLoop::handleConn, this));
    poller_->epoll_add(pwakeupChannel_, 0);
}

void EventLoop::handleConn()
{
    //poller_->epoll_mod(wakeupFd_, pwakeupChannel_, (EPOLLIN | EPOLLET | EPOLLONESHOT), 0);
    updatePoller(pwakeupChannel_, 0);
}


EventLoop::~EventLoop()
{
    //wakeupChannel_->disableAll();
    //wakeupChannel_->remove();
    close(wakeupFd_);
    t_loopInThisThread = NULL;
}

// 向wakeupFd_写入1，原本阻塞在epoll_wait()的线程会返回
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);
    if (n != sizeof one)
    {
        LOG<< "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

// 处理wakeupFd_上的读事件
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = readn(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
    //pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
}

// 在IO线程内执行某个用户任务回调
void EventLoop::runInLoop(Functor&& cb)
{
    if (isInLoopThread()) //如果用户在当前IO线程调用，回调会进行
        cb();
    else
        queueInLoop(std::move(cb));// 在其他线程中调用，cb会被加入队列中，IO线程会被唤醒，来调用这个cb
}

// 将cb放入队列中，并在必要时唤醒IO线程，多线程操作需要加锁
void EventLoop::queueInLoop(Functor&& cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }

    // 如果调用queueInLoop()的函数的线程不是IO线程 或者 正在执行pending functor 唤醒线程
    if (!isInLoopThread() || callingPendingFunctors_)
        wakeup();
}

/* EventLoop类的核心功能函数loop() */
// 在无限循环中
// 1. 通过调用方法Epoll:poll()，获取当前就绪的Channel指针数组
// 2. 对每个就绪的fd，调用handleEvents()处理其上就绪的事件
// 3. 处理queueInLoop上的回调
// 4. 处理超时事件
void EventLoop::loop()
{
    assert(!looping_); 
    assert(isInLoopThread()); // 保证每个线程只能有一个EventLoop
    looping_ = true;
    quit_ = false;
    //LOG_TRACE << "EventLoop " << this << " start looping";
    //typedef std::shared_ptr<Channel> SP_Channel;
    std::vector<SP_Channel> ret; // IO复用类Epoll:poll()，返回的当前就绪的Channel指针数组
    while (!quit_)
    {
        //cout << "doing" << endl;
        ret.clear();
		/* 关键步骤1：获取IO复用的结果，当前就绪的Channel指针数组 */
        ret = poller_->poll();

		/* 关键步骤2：*/
        eventHandling_ = true;
        for (auto &it : ret)
            it->handleEvents();
        eventHandling_ = false;

		/* 关键步骤3：*/
        doPendingFunctors();

		/* 关键步骤4：*/
        poller_->handleExpired();
    }
    looping_ = false;
}

// 执行回调列表中的回调函数
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

	// 临界区，把回调列表swap()到局部变量中，减小了临界区的长度，也避免了死锁
    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

	// 依次调用Functor
    for (size_t i = 0; i < functors.size(); ++i)
        functors[i]();
    callingPendingFunctors_ = false;
}

// 退出loop循环
void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}