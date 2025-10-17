#pragma once

#include<sys/epoll.h>
#include<vector>

#include"Poller.h"


//此epoll采用LT模式
class EPollPoller:public Poller
{
private:
    using EventList = std::vector<epoll_event>;
    static const int kEpollEventListSize=32;
    

    EventList event_list_;//用于装载epoll_wait返回的epoll_event集合

    int epoll_fd_;//epoll 的fd


    //更新对应的channel，内部调用epoll_ctl
    void update(int operation,Channel*channel);

    //将event_list_中的内容填充到一个一个的channel中
    void fillActiveChannels(ChannelList *active_channels) const;
    
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller();

    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

    Timestamp poll(int time_out,ChannelList*active_channels) override;
};

