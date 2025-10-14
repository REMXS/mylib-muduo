#include<sys/eventfd.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<memory>

#include"EventLoop.h"
#include"Logger.h"
#include"Channel.h"
#include"Poller.h"

//防止一个线程创建多个eventloop
thread_local EventLoop* t_loopInThisThread=nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs=10000; //单位为ms ，10000为10s



/*  创建一个eventfd
    EFD_NONBLOCK 表示设置为非阻塞
    EFD_CLOEXEC 表明在执行exec函数创建新进程的时候，不继承此fd
    因为eventfd用于进程中线程之间的通信，创建新进程线程的上下文不存在，
    所以eventfd要关闭防止意外的行为
 */
int createEventFd()
{
    int evtfd=::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
    if(evtfd<0)
    {
        LOG_FATAL("failed to create eventfd,errno: %d",errno)
    }
    return evtfd;
}

EventLoop::EventLoop()
    :looping_(false)
    ,quit_(false)
    ,thread_id_(CurrentThread::tid())
    ,poller_(Poller::newDefaultPoller(this))
    ,calling_pending_functors_(false)
    ,wakeup_fd_(createEventFd())
    ,wakeup_channel(std::make_unique<Channel>(this,wakeup_fd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, this->thread_id_);

    //遵循一个线程一个循环的设计规则,如果t_loopInThisThread不为空，则当前线程已有一个实例
    if(t_loopInThisThread)
    {
        LOG_FATAL("another eventLoop %p already exists in this thread %d",t_loopInThisThread,this->thread_id_)
    }
    t_loopInThisThread=this;

    //设置eventfd的处理函数
    this->wakeup_channel->setReadCallback([this](Timestamp){
        this->handleRead();
    });
    this->wakeup_channel->enableReading();
}

EventLoop::~EventLoop()
{

}

//将内部的poller_隐藏，对外部只暴露封装后的接口，更容易扩展，更直观简单
void EventLoop::updateChannel(Channel* channel)
{
    this->poller_->updateChannel(channel);
}


void EventLoop::removeChannel(Channel* channel)
{
    this->poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    return this->poller_->hasChannel(channel);
}


void EventLoop::handleRead()
{

}

void EventLoop::doingPendingFunctors()
{

}


void EventLoop::loop()
{

}


void EventLoop::quit()
{

}


void EventLoop::runInLoop(Functor cb)
{

}

void EventLoop::queueInLoop(Functor cb)
{

}

void EventLoop::wakeUp()
{

}