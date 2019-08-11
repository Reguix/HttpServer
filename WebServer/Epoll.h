// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "Channel.h"
#include "HttpData.h"
#include "Timer.h"
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <memory>

/* 
      只负责IO复用
      poll函数是Epoll的核心功能，获取当前活动的IO事件，返回就绪的Channel对象

      Epoll内部维护一个内核事件表，所有fd和Channel的对应关系，事件管理类TimerManager对象，并封装了函数epoll_ctl()和epoll_wait()
      
*/
class Epoll
{
public:
    Epoll();
    ~Epoll();
    void epoll_add(SP_Channel request, int timeout);
    void epoll_mod(SP_Channel request, int timeout);
    void epoll_del(SP_Channel request);
    std::vector<std::shared_ptr<Channel>> poll(); // 核心功能
    std::vector<std::shared_ptr<Channel>> getEventsRequest(int events_num);
	int getEpollFd()
    {
        return epollFd_;
    }
	/* 后面这两个函数用来处理定时器 本不应该放在这个类中 */

	// 增加定时器
    void add_timer(std::shared_ptr<Channel> request_data, int timeout);

	// 处理超时
    void handleExpired();
	
private:
    static const int MAXFDS = 100000; // IO复用管理的fd数目的上限
    int epollFd_; // epfd内核事件表描述符
    std::vector<epoll_event> events_; // epoll_event结构体数组，从内核事件表将所有就绪的事件复制到此
    std::shared_ptr<Channel> fd2chan_[MAXFDS]; // fd To（对应） Channel的数组，相当于一个固定大小的map
    std::shared_ptr<HttpData> fd2http_[MAXFDS]; // fd To（对应）Http的数组，相当于一个固定大小的map
    TimerManager timerManager_; // 定时器管理类 本不应该在这里
};