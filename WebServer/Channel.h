// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "Timer.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>
#include <functional>
#include <sys/epoll.h>


class EventLoop;
class HttpData;

/*
    Channel是selectable IO channel，负责注册和响应IO事件
	每个Channel对象自始至终只属于一个EventLoop， 因此每个Channel对象都只属于一个IO线程
	每个Channel对象自始至终只负责一个文件描述符（fd）的IO事件分发，但它不拥有这个fd，也不会在析构时关闭fd
	Channel会把不同的IO事件分发为不同的回调，例如ReadCallback、WriteCallback、errorCallback

	这里的Channel把muduo中的Acceptor（接收器，用于服务端接受连接）也揉进了这个类中，增加了connCallback
	把TcpConnection(TCP连接器，用于处理连接)也揉进了这个类中，增加了holder，

	handleEvent()是Channel的核心，它由EventLoop::loop()调用，它的功能是根据revents_的值分别调用不同的用户回调


	Channel类的作用是把fd 和fd感兴趣的IO事件，和fd上就绪的IO事件，以及对相应的IO事件进行处理的函数封装到一起
*/

class Channel
{
private:
    typedef std::function<void()> CallBack;
    EventLoop *loop_; // 自始至终属于一个EventLoop管理
    int fd_; // Channel管理的fd
    __uint32_t events_; // epoll或poll的结构体(epoll_event或pollfd)中events变量, 保存了fd上感兴趣的IO事件
    __uint32_t revents_; // 目前fd上就绪的事件，是由Epoll类的poll方法获得
    __uint32_t lastEvents_;

    // 方便找到上层持有该Channel的对象
    std::weak_ptr<HttpData> holder_;

private:
    int parse_URI();
    int parse_Headers();
    int analysisRequest();

    CallBack readHandler_;
    CallBack writeHandler_;
    CallBack errorHandler_;
    CallBack connHandler_;

public:
    Channel(EventLoop *loop);
    Channel(EventLoop *loop, int fd);
    ~Channel();
    int getFd(); // 返回管理的fd
    void setFd(int fd); // 设置管理的fd

    // 
    void setHolder(std::shared_ptr<HttpData> holder)
    {
        holder_ = holder;
    }
    std::shared_ptr<HttpData> getHolder()
    {
        std::shared_ptr<HttpData> ret(holder_.lock());
        return ret;
    }

    void setReadHandler(CallBack &&readHandler)
    {
        readHandler_ = readHandler;
    }
    void setWriteHandler(CallBack &&writeHandler)
    {
        writeHandler_ = writeHandler;
    }
    void setErrorHandler(CallBack &&errorHandler)
    {
        errorHandler_ = errorHandler;
    }
    void setConnHandler(CallBack &&connHandler)
    {
        connHandler_ = connHandler;
    }

    // handleEvent()是Channel的核心，它由EventLoop::loop()调用，它的功能是根据revents_的值分别调用不同的用户回调
    void handleEvents()
    {
        events_ = 0;
        if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) // 挂起（管道的写端被关闭，读端将收到EPOLLHUP事件） 并且 无数据可读
        {
            events_ = 0;
            return;
        }
        if (revents_ & EPOLLERR) // 错误
        {
            if (errorHandler_) errorHandler_();
            events_ = 0;
            return;
        }
        if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) // 数据可读 或 高优先级数据可读（TCP带外数据） 或 对方关闭了写操作
        {
            handleRead();
        }
        if (revents_ & EPOLLOUT) // 数据可写
        {
            handleWrite();
        }
        handleConn(); // 处理连接
    }
    void handleRead();
    void handleWrite();
    void handleError(int fd, int err_num, std::string short_msg);
    void handleConn();

    void setRevents(__uint32_t ev) // 设置fd上的就绪事件
    {
        revents_ = ev;
    }

    void setEvents(__uint32_t ev) // 设置fd上的感兴趣事件
    {
        events_ = ev;
    }
    __uint32_t& getEvents() // 返回感兴趣事件
    {
        return events_;
    }

    bool EqualAndUpdateLastEvents() // 更新上次注册的事件
    {
        bool ret = (lastEvents_ == events_);
        lastEvents_ = events_;
        return ret;
    }

    __uint32_t getLastEvents()
    {
        return lastEvents_;
    }

};

typedef std::shared_ptr<Channel> SP_Channel;