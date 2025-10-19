#pragma once

#include<memory>
#include<functional>
#include<string>
#include<atomic>
#include<mutex>
#include<condition_variable>

#include"Thread.h"
#include"EventLoop.h"
#include"noncopyable.h"



class EventLoopThread:noncopyable
{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;
private:
    std::shared_ptr<Thread>thread_;
    EventLoop* loop_;
    std::atomic_bool exiting_;
    ThreadInitCallback call_back_;//执行的初始化回调函数，用于在loop中做一些初始操作

    //因为在子线程中需要创建loop，而主线程又需要得到这个loop，从而需要loop_这个中间量传递，因此要加锁
    std::mutex mtx_;
    std::condition_variable cv_;

    std::string name_;

    void ThreadFunc();//要在子线程中执行的函数

public:
    EventLoop* startLoop();//因为主eventloop在分发连接的时候需要调用其它loop中的wakeup函数，所以主loop需要知道其它loop对象


    EventLoopThread(const ThreadInitCallback&cb,const std::string&name="");
    ~EventLoopThread();
};


