#include<unistd.h>
#include<errno.h>
#include<string.h>



#include"EPollPoller.h"
#include"Channel.h"
#include"Logger.h"

//以下为channel中index中的值的含义
const int kNew=-1; //表示channel还没添加到channel中
const int kAdd=1; //表示channel添加到了channel中被监听
const int kDelete=2; //表示channel被监听的事件在epoll中被删除，但是channel在保留在channels_映射中,功能类似挂起


EPollPoller::EPollPoller(EventLoop* loop)
    :Poller(loop)
    ,epoll_fd_(epoll_create1(EPOLL_CLOEXEC))//表明再执行exec时关闭文件描述符
    ,event_list_(kEpollEventListSize)
{
    if(epoll_fd_<0)
    {
        LOG_FATAL("epoll_create error %d\n",errno)
    }
}

EPollPoller::~EPollPoller()
{
    ::close(this->epoll_fd_);
}

void EPollPoller::updateChannel(Channel* channel) 
{
    int idx=channel->index();
    int cfd=channel->fd();
    
    LOG_DEBUG("func=%s => fd=%d events=%d index=%d", __FUNCTION__, channel->fd(), channel->events(), idx);

    //channel不在epoll中被监听
    if(idx==kNew||idx==kDelete)
    {
        //防止新加入epoll或者暂停监听的channel忘记设置监听事件
        if(channel->isNoneEvent())
        {
            LOG_ERROR("EPollPoller::updateChannel: fd %d forgot to add events, failed to update channel",cfd)
            return;
        }

        //新加入的channel，还没有在channels_中记录
        if(idx==kNew)
        {
            this->channels_[cfd]=channel;
        }
        //被暂停的channel，在channels_已经有记录
        else if(idx==kDelete)
        {
        }

        update(EPOLL_CTL_ADD,channel);
        channel->setIndex(kAdd);
    }
    //channel在epoll中
    else
    {   
        //暂停监听epoll
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL,channel);
            channel->setIndex(kDelete);
        }
        //更新监听事件
        else
        {
            update(EPOLL_CTL_MOD,channel);
        }
    }
}

void EPollPoller::removeChannel(Channel* channel) 
{
    int idx=channel->index();
    int fd=channel->fd();

    if(idx==kNew) return;

    LOG_INFO("func=%s => fd=%d", __FUNCTION__, fd);
    this->channels_.erase(fd);//清除登记信息

    //如果channel在epoll中，清除epoll中的channel
    if(idx==kAdd)
    {
        update(EPOLL_CTL_DEL,channel);
    }
    
    channel->setIndex(kNew);
}

//一次epoll wait 循环
Timestamp EPollPoller::poll(int time_out,ChannelList*active_channels)
{
    LOG_DEBUG("func=%s => fd total count:%lu", __FUNCTION__, channels_.size());

    int num_events= epoll_wait(this->epoll_fd_,&*this->event_list_.begin(),static_cast<int>(event_list_.size()),time_out);
    Timestamp st=Timestamp::now();

    if(num_events>0)
    {   
        LOG_DEBUG("%d events happend",num_events)
        fillActiveChannels(num_events,active_channels);
    }
    else if(num_events==0)
    {
        LOG_DEBUG("%s time out ! ",__FUNCTION__)
    }
    else
    {
        /* EINTR触发条件：当进程阻塞在系统调用（如 read、write、epoll_wait）时，
        如果收到信号（signal），系统调用会立即返回 -1，并设置 errno 为 EINTR，而不是继续等待。
        这不是真正的错误，而是操作系统处理信号（如定时器或用户信号）的正常行为。 */
        if(errno!=EINTR)
        {
            LOG_ERROR("EPollPoller::poll error! %d",errno)
        }
    }
    
    return st;
}
void EPollPoller::update(int operation,Channel*channel)
{
    epoll_event ev;
    ::memset(&ev,0,sizeof(ev));

    ev.data.ptr=channel;
    ev.events=channel->events();

    if(::epoll_ctl(this->epoll_fd_,operation,channel->fd(),&ev)<0)
    {
        if(operation==EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error %d",errno)
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error %d",errno)
        }
    }

}

//填写活跃的链接
void EPollPoller::fillActiveChannels(int num_events,ChannelList *active_channels) const
{
    for(int i=0;i<num_events;++i)
    {
        Channel* channel=static_cast<Channel*>(event_list_[i].data.ptr);
        channel->set_revent(event_list_[i].events);
        active_channels->emplace_back(channel);
    }
}