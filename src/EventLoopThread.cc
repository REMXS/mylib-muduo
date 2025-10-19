#include<assert.h>


#include"EventLoopThread.h"


EventLoopThread::EventLoopThread(const ThreadInitCallback&cb,const std::string&name)
    :thread_(nullptr)
    ,loop_(nullptr)
    ,exiting_(false)
    ,call_back_(cb)
    ,name_(name)
{

}

EventLoopThread::~EventLoopThread()
{
    exiting_=true;
    if(loop_!=nullptr)
    {
        loop_->quit();
    }
    if(thread_!=nullptr)
    {
        thread_->join();
    }
}

void EventLoopThread::ThreadFunc()
{
    EventLoop loop;
    if(call_back_)
    {
        call_back_(&loop);
    }

    if(!exiting_)
    {
        std::lock_guard<std::mutex>lock(mtx_);
        this->loop_=&loop;
        cv_.notify_one();
    }
    else
    {
        return;
    }
    loop.loop();

    std::lock_guard<std::mutex>lock(mtx_);
    loop_=nullptr;
}


EventLoop*  EventLoopThread::startLoop()
{
    assert(thread_==nullptr&&"there is already a object in thread_, but it should not happen");
    
    thread_=std::make_shared<Thread>([this](){
        this->ThreadFunc();
    },name_);

    //启动子线程
    thread_->start();
    //等待子线程创建loop对象，获取这个loop对象的指针
    {   
        std::unique_lock<std::mutex>lock(mtx_);
        cv_.wait(lock,[this]()->bool{
            return loop_!=nullptr;
        });
    }
    
    return loop_;
}