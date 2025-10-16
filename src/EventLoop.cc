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
//因为这个变量仅供内部判断使用，所以定义在实现文件，不对外暴露
//对外使用new 创建的eventloop对象而不使用t_loopInThisThread
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
    this->wakeup_channel->disableAll();//注销所有监听的事件
    this->wakeup_channel->remove();//从poller中移除此fd
    ::close(this->wakeup_fd_);//关闭fd
    t_loopInThisThread=nullptr;//重新设置此线程的eventloop对象为空指针
}

/* 将内部的poller_隐藏，所有的功能都不直接依赖成员变量，而是全部做成成员函数，
对外部只暴露封装后的接口，更容易扩展，更直观简单 
当内部成员变量的实现细节更改是，也只需更改相关的函数，不用更改具体的代码流程
*/
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
    uint64_t data=0;
    ssize_t n=read(wakeup_fd_,&data,sizeof(data));
    if(n!=sizeof(data))
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8\n",n)
    }
}


/* 唤醒loop所在的线程，向这个loop的wakeup_fd_写入数据，
wakeupChannel就发生读事件(poller检测到读事件，调用 handleRead() 读取数据)
因此这个loop被唤醒
 */
void EventLoop::wakeUp()
{
    uint64_t data=1;
    //成功返回写入的字节数，失败返回-1，并设置errno
    ssize_t n=write(this->wakeup_fd_,&data,sizeof(data));
    if(n!=sizeof(data))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n",n)
    }
}

void EventLoop::doingPendingFunctors()
{
    //先设置正在执行追加任务的标志为true
    this->calling_pending_functors_=true;
    std::vector<Functor>tasks;
    {   
        /* 用一个临时的容器将队列中的函数转移，避免了在临界区执行过多的时间，从而减慢了运行的效率
            也防止了pendingFunctors_中有追加任务的操作导致重复上锁而导致死锁的问题
         */
        std::lock_guard<std::mutex>lock(this->mtx_);
        tasks.swap(this->pendingFunctors_);
    }
    for(const auto&task:tasks)
    {
        task();
    }
    this->calling_pending_functors_=false;
}

void EventLoop::runInLoop(Functor cb)
{
    //判断loop对象是否是在当前线程中，如果是，直接执行，否则将cb加入队列，然后唤醒对应的loop执行函数
    if(isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::lock_guard<std::mutex>lock(mtx_);
        this->pendingFunctors_.emplace_back(std::move(cb));
    }

    /* 如果是loop对象是在当前线程中，则一定在执行一次loop之后的doingPendingFunctors操作，
       从而calling_pending_functors_一定为true，此时又向队列中加入函数，则下次循环时也需要唤醒
       如果loop对象不在在当前线程中，则直接唤醒
    */
    if(!isInLoopThread()||calling_pending_functors_)
    {
        this->wakeUp();
    }
    
}

void EventLoop::loop()
{
    this->looping_=true;
    this->quit_=false;

    LOG_INFO("EventLoop %p start looping\n", this);
    while(!quit_)
    {
        //先清空上次残留在active_channels的任务
        active_channels.clear();

        this->pollReturnTime_= poller_->poll(kPollTimeMs,&active_channels);
        //执行所唤醒channel的操作
        for(auto channel:active_channels)
        {
            channel->handleEvent(this->pollReturnTime_);
        }
        //执行其它对象追加到这个对象的任务
        this->doingPendingFunctors();
    }
    looping_=false;
    LOG_INFO("EventLoop %p stop looping\n", this);
}

/**
 * 退出事件循环
 * 1. 如果loop在自己的线程中调用quit成功了 说明当前线程已经执行完毕了loop()函数的poller_->poll并退出
 * 2. 如果不是当前EventLoop所属线程中调用quit退出EventLoop 需要唤醒EventLoop所属线程的epoll_wait
 *
 * 比如在一个subloop(worker)中调用mainloop(IO)的quit时 需要唤醒mainloop(IO)的poller_->poll 让其执行完loop()函数
 *
 * ！！！ 注意： 正常情况下 mainloop负责请求连接 将回调写入subloop中 通过生产者消费者模型即可实现线程安全的队列
 * ！！！       但是muduo通过wakeup()机制 使用eventfd创建的wakeupFd_ notify 使得mainloop和subloop之间能够进行通信
 **/
void EventLoop::quit()
{
    quit_=true;
    if(!isInLoopThread())
    {
        wakeUp();
    }
}




