#pragma once
#include <memory>
#include <map>
#include <set>
#include <vector>

#include "Channel.h"
#include "noncopyable.h"
#include "Callbacks.h"


class EventLoop;
class Timer;
class TimerId;


class TimerQueue: noncopyable
{
private:
    using TimerPtr = std::unique_ptr<Timer>;
    using Entry = std::pair<Timestamp,int64_t>;
    using TimerList = std::map<Entry,TimerPtr>;

    using ActiveTimer = std::pair<Timer*,int64_t>;
    using ActiveTimerSet = std::set<ActiveTimer>;


    EventLoop* loop_;
    const int timer_fd;     // must be initialized before channel uses it
    Channel timer_fd_channel;

    TimerList timer_list_;
    ActiveTimerSet active_timer_set_;
    ActiveTimerSet canceling_timer_set_;
    bool is_running_callback_;

    void addTimerInLoop(Timer* timer);
    void cancelTimerInLoop(TimerId timer_id);

    //timer_fd_channel的读回调函数
    void handleRead();

    //获取过期的定时器
    std::vector<Entry>getExpiredTimers(Timestamp now);
    //处理过期的定时器
    void reset(const std::vector<Entry>expired_timers,Timestamp now);
    //内部插入timer的具体实现
    bool insert(std::unique_ptr<Timer> timer);
    //内部删除timer的具体实现
    void erase(ActiveTimer del_timer);


public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerId addTimer(TimerCallback cb,Timestamp when, double interval);
    void cancelTimer(TimerId timer_id);
};


