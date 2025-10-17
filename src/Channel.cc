#include<sys/epoll.h>

#include"Channel.h"
#include"Logger.h"
#include"EventLoop.h"

const int Channel::kReadEvent=EPOLLIN|EPOLLPRI;//读事件
const int Channel::kWriteEvent=EPOLLOUT;//写事件
const int Channel::kNoneEvent=0; //空事件


Channel::Channel(EventLoop*loop,int fd)
    :fd_(fd)
    ,loop_(loop)
    ,events_(0)
    ,revent_(0)
    ,index_(-1)
    ,tied_(false)
{
}

Channel::~Channel()
{
}

void Channel::update()
{
    //通过所属的eventloop来更新channel的状态
    loop_->updateChannel(this);
}

void Channel::remove()
{
    //通过所属的eventloop来删除channel
    loop_->removeChannel(this);
}


//绑定上层的对象，在调用上层注册的回调函数的时候，通过tie_检查上层对象是否死亡，防止出错
void Channel::tie(const std::shared_ptr<void>&sp)
{
    this->tie_=sp;
    this->tied_=true;
}

void Channel::handleEvent(Timestamp receive_time)
{
    //如果回调函数依赖上层对象则尝试升级，否则直接调用
    if(tied_){
        //通过将weak ptr升级为 shared ptr，检查上层对象是否死亡
        std::shared_ptr<void>guard=tie_.lock();
        
        //如果升级成功则调用回调函数处理事件，否则什么都不做
        if(guard)
        {
            this->handleEventWithGuard(receive_time);
        }
    }
    else
    {
         this->handleEventWithGuard(receive_time);
    }
}

void Channel::handleEventWithGuard(Timestamp receive_time)
{
    LOG_INFO("channel handleEvent revents:%d\n",this->revent_);

    //根据实际返回的真实的事件调用不同的回调函数
    
    //关闭
    if((this->revent_&EPOLLHUP)&&(!(this->revent_&EPOLLIN)))
    {
        if(this->close_callback_)
        {
            this->close_callback_();
        }
    }
    //读
    if(this->revent_&(EPOLLIN|EPOLLPRI))
    {
        if(this->read_callback_)
        {
            this->read_callback_(receive_time);
        }
    }
    //写
    if(this->revent_&EPOLLOUT)
    {
        if(this->write_callback_)
        {
            this->write_callback_();
        }
    }
    //错误
    if(this->revent_&EPOLLERR)
    {
        if(this->error_callback_)
        {
            this->error_callback_();
        }
    }
}