#pragma once

#include<memory>
#include<functional>

#include"noncopyable.h"
#include"Timestamp.h"

class EventLoop;

/* 
//以下为channel中index中的值的含义
const int kNew=-1; //表示channel还没添加到channel中
const int kAdd=1; //表示channel添加到了channel中被监听
const int kDelete=2; //表示channel被监听的事件在epoll中被删除，但是channel在保留在channels_映射中,功能类似挂起 
*/

class Channel:noncopyable
{
public:
    using EventCallBack=std::function<void()>;
    using ReadEventCallBack=std::function<void(Timestamp)>;
private:
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; //channel 所在的loop
    const int fd_; //channel 绑定的fd
    int events_; //注册fd所有感兴趣的事件
    int revent_;//当前循环实际返回的事件
    int index_;

    //因为channel调用的回调函数依赖上层的对象，这个变量通过将上层的weak_ptr指针升级成shared_ptr的成败来判断上层对象是否存活。
    std::weak_ptr<void>tie_;
    bool tied_;


    ReadEventCallBack read_callback_;
    EventCallBack write_callback_;
    EventCallBack close_callback_;
    EventCallBack error_callback_;

    //在loop中更新channel的状态
    void update();
    //handleEvent的内部执行的函数
    void handleEventWithGuard(Timestamp receive_time);
public:
    Channel(EventLoop*loop,int fd);
    ~Channel();
   
    //在poller中的fd得到通知，则调用此函数处理事件
    void handleEvent(Timestamp receive_time);


    //设置回调函数
    void setReadCallback(ReadEventCallBack cb){this->read_callback_=std::move(cb);}
    void setWriteCallback(EventCallBack cb){this->write_callback_=std::move(cb);}
    void setCloseCallback(EventCallBack cb){this->close_callback_=std::move(cb);}
    void setErrorCallback(EventCallBack cb){this->error_callback_=std::move(cb);}


    int fd(){return this->fd_;}
    int events(){return this->events_;}

    int index(){return this->index_;}
    void setIndex(int index){this->index_=index;} 

    void set_revent(int revent){this->revent_=revent;}


    //在channel绑定上层的tcp connection时调用
    void tie(const std::shared_ptr<void>&sp);


    //设置fd对应的事件，使poller对象监听对应的事件，效果相当于event_ctl 中的add，del
    void enableReading(){this->events_|=kReadEvent;update();}
    void disableReading(){this->events_&=~kReadEvent;update();}
    void enableWriting(){this->events_|=kWriteEvent;update();}
    void disableWriting(){this->events_&=~kWriteEvent;update();}
    void disableAll(){this->events_=kNoneEvent;update();}

    //返回fd当前对应的事件状态
    bool isReading(){return this->events_&kReadEvent;}
    bool isWriting(){return this->events_&kWriteEvent;}
    bool isNoneEvent(){return this->events_==kNoneEvent;}

    EventLoop* ownerLoop(){return this->loop_;}
    void remove();

};


