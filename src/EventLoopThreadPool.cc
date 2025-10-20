
#include<cassert>

#include"EventLoopThreadPool.h"
#include"EventLoopThread.h"
#include"EventLoop.h"


EventLoopThreadPool::EventLoopThreadPool(EventLoop*base_loop,std::string name)
    :base_loop_(base_loop)
    ,name_(name)
    ,started_(false)
    ,next_(0)
    ,num_threads_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
}


void EventLoopThreadPool::setNumThreads(int num_threads)
{
    assert(!started_&&"the thread pool has already started,don't fix the num_threads!");
    num_threads_=num_threads;
}


void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    started_=true;
    for(int i=0;i<num_threads_;++i)
    {
        std::string buf=name_+std::to_string(i);
        
        threads_.emplace_back(std::make_unique<EventLoopThread>(cb,buf));
        loops_.emplace_back(threads_.back()->startLoop());
    }
    //如果不使用多线程，则base_loop_ 也充当subloop，执行初始化函数
    if(num_threads_==0&&cb)
    {
        cb(base_loop_);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{

    EventLoop* loop=base_loop_;

    //如果不使用多线程就一直返回 baseloop
    if(!loops_.empty())
    {
        loop=loops_[next_];
        next_=(next_+1)%num_threads_;
    }

    return loop;
}


std::vector<EventLoop*>EventLoopThreadPool::getAllLoops()
{
    if(!loops_.empty())
    {
        return loops_;
    }
    else
    {
        return std::vector<EventLoop*>(1,base_loop_);
    }
}