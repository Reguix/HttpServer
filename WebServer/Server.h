// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "EventLoop.h"
#include "Channel.h"
#include "EventLoopThreadPool.h"
#include <memory>

// HttpServer服务器类
class Server
{
public:
	/* 
	    muduo的做法是实现一个底层的TcpServer类和TcpConnection类，TcpServer中有一个TcpConnection指针，
	    然后在自定义的HttpServer类的构造函数中往TcpServer对象中注册回调函数，
	    这里做法相当于把TcpServer类揉进了本类HttpServer的实现中，
	    所以没有了注册回调函数的步骤，直接处理新连接newConnection，和本连接Connection
    */
    Server(EventLoop *loop, int threadNum, int port); 
    ~Server() { }
    EventLoop* getLoop() const { return loop_; } // 返回EventLoop对象指针
    void start(); // muduo这里调用TcpServer类的start()方法，此处是实现TcpServer类的start()方法
    void handNewConn(); // 处理新连接
    void handThisConn() { loop_->updatePoller(acceptChannel_); } // 处理连接

private:
    /* 
        TcpServer类中拥有一个主事件循环对象EventLoop、一个接受器Acceptor、一个线程池对象EventLoopThreadPool
    */
    EventLoop *loop_; // 只用来接受新连接，
    int threadNum_; // 多线程线程数
    std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_; // 线程池unique指针，创建IO线程池，把TcpConnection分派到不同的EventLoop上
    bool started_;
    std::shared_ptr<Channel> acceptChannel_; // 接受器指针，相当于Acceptor，用于服务器接受连接，用来获取新连接的fd
    int port_; // 端口
    int listenFd_; // 监听描述符
    static const int MAXFDS = 100000; // 最大描述符限制
};