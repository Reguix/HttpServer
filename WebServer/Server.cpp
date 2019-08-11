// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Server.h"
#include "base/Logging.h"
#include "Util.h"
#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// 构造函数，初始化成员变量loop_、threadNum_、port、started_(false)
Server::Server(EventLoop *loop, int threadNum, int port)
:   loop_(loop),
    threadNum_(threadNum),
    eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)), // 创建线程池
    started_(false),
    acceptChannel_(new Channel(loop_)), // 接受器Channel
    port_(port),
    listenFd_(socket_bind_listen(port_)) // 初始化监听描述符
{
    acceptChannel_->setFd(listenFd_); // 设置接受器的fd
    handle_for_sigpipe(); // 处理SIGPIPE(向读端关闭的管道或socket连接中写数据)信号
    if (setSocketNonBlocking(listenFd_) < 0) // 设置fd为非阻塞
    {
        perror("set socket non block failed");
        abort();
    }
}

// 开启线程池，初始化接收器，在baseLoop中注册接受器
void Server::start()
{
    eventLoopThreadPool_->start(); // 开启线程池
    //acceptChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);

	/* 下面这部分原本应该由Acceptor类负责 */
    acceptChannel_->setEvents(EPOLLIN | EPOLLET); // 注册epoll_evevts中events事件
    acceptChannel_->setReadHandler(bind(&Server::handNewConn, this)); // 设置acceptChannel_::handleRead()回调函数
    acceptChannel_->setConnHandler(bind(&Server::handThisConn, this)); // 设置acceptChannel_::handleConn()回调函数
    loop_->addToPoller(acceptChannel_, 0);

	
    started_ = true;
}

// 接受新连接，分配新连接到子线程中
// handNewConn原本应该是由Acceptor负责的，并在Acceptor的handleRead中调用
// 这里用acceptChannel代替了Acceptor，所以在start()函数中acceptChannel_->setReadHandler(bind(&Server::handNewConn, this))
void Server::handNewConn()
{
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);
    int accept_fd = 0;
    while((accept_fd = accept(listenFd_, (struct sockaddr*)&client_addr, &client_addr_len)) > 0)
    {

	    /* 多线程的核心，轮询调度，从线程池中取出子线程的事件循环，把新连接交由子线程处理 */
        EventLoop *loop = eventLoopThreadPool_->getNextLoop();


		
        LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port);
        // cout << "new connection" << endl;
        // cout << inet_ntoa(client_addr.sin_addr) << endl;
        // cout << ntohs(client_addr.sin_port) << endl;
        /*
        // TCP的保活机制默认是关闭的
        int optval = 0;
        socklen_t len_optval = 4;
        getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
        cout << "optval ==" << optval << endl;
        */
        // 限制服务器的最大并发连接数
        if (accept_fd >= MAXFDS)
        {
            close(accept_fd);
            continue;
        }
        // 设为非阻塞模式
        if (setSocketNonBlocking(accept_fd) < 0)
        {
            LOG << "Set non block failed!";
            //perror("Set non block failed!");
            return;
        }

        setSocketNodelay(accept_fd);
        //setSocketNoLinger(accept_fd);

        // 新建的连接由HttpData类管理，这里的HttpData相当于TcpConnect
        shared_ptr<HttpData> req_info(new HttpData(loop, accept_fd));
        req_info->getChannel()->setHolder(req_info);
		// 调用newEvent将req_info中的channel注册到IO子线程
		// 感觉这里应该调用loop->runInLoop()
		loop->queueInLoop(std::bind(&HttpData::newEvent, req_info));
    }
    acceptChannel_->setEvents(EPOLLIN | EPOLLET); // 再次设置acceptChannel的事件
}