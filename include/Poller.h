#pragma once

#include<vector>
#include<unordered_map>

#include"noncopyable.h"
#include"Timestamp.h"

class Channel;
class EventLoop;

class Poller
{
public:
    using ChannelList=std::vector<Channel*>;
private:
    EventLoop*loop_;
    
protected:
    using ChannelMap=std::unordered_map<int,Channel*>;
    ChannelMap channels_;
public:
    //静态工厂方法，返回具体的poller实例
    static Poller* newDefaultPoller(EventLoop*loop);

    //判断poller中是否含有指定的channel
    bool hasChannel(Channel* channel);

    //更新channel
    virtual void updateChannel(Channel* channel)=0;
    //删除channel
    virtual void removeChannel(Channel* channel)=0;

    //一次事件循环
    virtual Timestamp poll(int time_out,ChannelList*active_channels)=0;


    Poller(EventLoop*loop);
    ~Poller()=default;
};


