// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "HttpData.h"
#include "base/noncopyable.h"
#include "base/MutexLock.h"
#include <unistd.h>
#include <memory>
#include <queue>
#include <deque>

class HttpData;

/*
      定时器实现
      
*/
// 定时器节点 timeout的单位是毫秒
class TimerNode
{
public:
    TimerNode(std::shared_ptr<HttpData> requestData, int timeout);
    ~TimerNode();
    TimerNode(TimerNode &tn);
    void update(int timeout);
    bool isValid();
    void clearReq();
    void setDeleted() { deleted_ = true; }
    bool isDeleted() const { return deleted_; }
    size_t getExpTime() const { return expiredTime_; } 

private:
    bool deleted_;
    size_t expiredTime_; 
    std::shared_ptr<HttpData> SPHttpData;
};

struct TimerCmp
{
    bool operator()(std::shared_ptr<TimerNode> &a, std::shared_ptr<TimerNode> &b) const
    {
        return a->getExpTime() > b->getExpTime();
    }
};



// 最小堆实现定时器
class TimerManager
{
public:
    TimerManager();
    ~TimerManager();
    void addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout);
    void handleExpiredEvent();

private:
    typedef std::shared_ptr<TimerNode> SPTimerNode; // 将定时节点的指针
    std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp> timerNodeQueue; // 用优先队列实现定时器
    //MutexLock lock;
};