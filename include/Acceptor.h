#pragma once

#include<functional>

#include<Channel.h>
#include"Socket.h"
#include"noncopyable.h"

class InetAddress;
class EventLoop;

class Acceptor :noncopyable
{
public:
    using NewConnectionCallBack=std::function<void(int sockfd,const InetAddress&)>;
private:
    EventLoop* loop_;//listenfd 所在的loop

    //socket和channel中绑定的fd就是listenfd，fd的声明周期和socket绑定，socket析构时，fd关闭
    Socket accept_socket_; //专门用于接听新连接的socket
    Channel accept_channel_;//专门用于接听新连接的channel

    bool listening_;

    NewConnectionCallBack connection_callback_;//用于处理新建立连接的函数


    void handRead();//listenfd 的读回调函数

public:
    Acceptor(EventLoop*loop,const InetAddress&addr,bool reuse);
    ~Acceptor();

    //设置listenfd的listen状态,并将其加入poller中进行监听
    void listen(); 
    bool isListening(){return listening_;}

    void setConnetionCallback(NewConnectionCallBack cb){connection_callback_=std::move(cb);}
};


