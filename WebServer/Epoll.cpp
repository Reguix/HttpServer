// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Epoll.h"
#include "Util.h"
#include "base/Logging.h"
#include <sys/epoll.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <queue>
#include <deque>
#include <assert.h>

#include <arpa/inet.h>
#include <iostream>
using namespace std;


const int EVENTSNUM = 4096; // 同时就绪fd数目的上限
const int EPOLLWAIT_TIME = 10000; //超时时间

typedef shared_ptr<Channel> SP_Channel;

Epoll::Epoll():
    epollFd_(epoll_create1(EPOLL_CLOEXEC)), //调用epoll_create1创建内核事件表
    events_(EVENTSNUM) // 结构体数组大小
{
    assert(epollFd_ > 0);
}
Epoll::~Epoll()
{ }


// 注册新描述符
// 在往内核表中注册fd时，添加了定时器
// 封装epoll_ctl( ,EPOLL_CTL_ADD, , )函数，将Channel中的感兴趣的事件注册到内核表中
void Epoll::epoll_add(SP_Channel request, int timeout)
{
    int fd = request->getFd();

	
    if (timeout > 0)
    {
        add_timer(request, timeout);
        fd2http_[fd] = request->getHolder();
    }
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();

    request->EqualAndUpdateLastEvents();

    fd2chan_[fd] = request;
    if(epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        perror("epoll_add error");
        fd2chan_[fd].reset();
    }
}


// 修改描述符状态
// 封装epoll_ctl( ,EPOLL_CTL_MOD, , )函数，修改内核表中之前注册的fd感兴趣的事件
// 
void Epoll::epoll_mod(SP_Channel request, int timeout)
{
    // 
    if (timeout > 0)
        add_timer(request, timeout);
    int fd = request->getFd();
    if (!request->EqualAndUpdateLastEvents()) // 判断是否有修改，不相等则有修改
    {
        struct epoll_event event;
        event.data.fd = fd;
        event.events = request->getEvents();
        if(epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0)
        {
            perror("epoll_mod error");
            fd2chan_[fd].reset();
        }
    }
}


// 从epoll中删除描述符
// 封装epoll_ctl( ,EPOLL_CTL_DEL, , )函数，删除内核表中之前注册的fd
void Epoll::epoll_del(SP_Channel request)
{
    int fd = request->getFd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getLastEvents();
    //event.events = 0;
    // request->EqualAndUpdateLastEvents()
    if(epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0)
    {
        perror("epoll_del error");
    }
    fd2chan_[fd].reset(); // 减少引用计数，指针置空
    fd2http_[fd].reset(); // 减少引用计数，指针置空
}



/* Epoll类的核心功能函数poll */
// 返回活跃事件数
// 调用epoll_wait函数，返回一个已就绪的Channel数组
std::vector<SP_Channel> Epoll::poll()
{
    while (true)
    {
        int event_count = epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
        if (event_count < 0)
            perror("epoll wait error");
        std::vector<SP_Channel> req_data = getEventsRequest(event_count); // 处理epoll_wait的返回结果，将就绪的事件，保存在channel数组中。
        if (req_data.size() > 0)
            return req_data;
    }
}

// 处理超时事件
void Epoll::handleExpired()
{
    timerManager_.handleExpiredEvent();
}

// 
// 获取就绪的fd及其上的就绪事件
std::vector<SP_Channel> Epoll::getEventsRequest(int events_num)
{
    std::vector<SP_Channel> req_data; // 要返回的就绪Channel指针数组
	// 遍历events_结构体数组
    for(int i = 0; i < events_num; ++i)
    {
        // 获取有事件产生的描述符
        int fd = events_[i].data.fd;

		// 与该fd对应的Channel对象的指针
        SP_Channel cur_req = fd2chan_[fd];
            
        if (cur_req)
        {
            cur_req->setRevents(events_[i].events); // 获取fd上的就绪事件，设置管理该fd的Channel对象上的就绪事件
            cur_req->setEvents(0); // 清空感兴趣的事件
            // 加入线程池之前将Timer和request分离
            //cur_req->seperateTimer();
            req_data.push_back(cur_req); // 将此Channel指针加入到要返回的就绪数组中
        }
        else
        {
            LOG << "SP cur_req is invalid";
        }
    }
    return req_data;
}

// 添加定时器
void Epoll::add_timer(SP_Channel request_data, int timeout)
{
    // 找到持有Channel对象的HttpData
    shared_ptr<HttpData> t = request_data->getHolder();
	// 调用
    if (t)
        timerManager_.addTimer(t, timeout);
    else
        LOG << "timer add fail";
}