#pragma once

#include<functional>
#include<vector>
#include<atomic>
#include<memory>
#include<mutex>

#include"CurrentThread.h"
#include"Timestamp.h"
#include"noncopyable.h"

class Channel;
class Poller;

class EventLoop:noncopyable
{
public:
    using Functor=std::function<void()>;
private:
    using ChannelLists=std::vector<Channel*>;

    //给wakeupfd注册的读处理函数，清空eventfd的计数代表了响应了此次请求
    void handleRead();
    //在每次循环结束的时候，执行任务队列中的额外任务，任务多位上层回调
    void doingPendingFunctors();

    ChannelLists active_channels;

    std::atomic_bool looping_;
    std::atomic_bool quit_;

    //拥有此eventloop的线程id，每一个eventloop对应一个线程，一个线程只允许存在一个eventloop
    const pid_t thread_id_;

    //每次循环调用poller时的时间点
    Timestamp pollReturnTime_; 

    std::unique_ptr<Poller>poller;


    //用于跨线程唤醒操作
    int wakeup_fd_;
    //当主loop有新注册的channel时，唤醒子loop添加新的channel
    std::unique_ptr<Channel*>wakeup_channel;
    //等待被执行的任务队列
    std::vector<Functor>pendingFunctors_;
    //此loop是否在执行任务队列中任务的标志
    std::atomic_bool calling_pending_functors_;
    //可能有其它线程在任务队列中追加任务，所以要加锁
    std::mutex mtx_;

public:
    //开启事件循环
    void loop();
    //停止事件循环
    void quit();

    //获取最近一次poller返回的时间
    Timestamp getPollReturnTime(){return this->pollReturnTime_;}

    //在指定loop中执行任务
    void runInLoop(Functor cb);
    //将任务追加到任务队列当中并唤醒loop执行任务
    void queueInLoop(Functor cb);

    //通过eventfd唤醒loop执行队列中的任务
    void wakeUp();

    //供eventloop方便调用的poller中的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    //判断此eventloop知否在当前线程中
    bool isInLoopThread(){return this->thread_id_==CurrentThread::tid();}


    
    //eventloop 在初始化的时候会自动创建poller，所以此处不用传参数
    EventLoop();
    ~EventLoop();
};




