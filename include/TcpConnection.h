#pragma once

#include <memory>
#include <string>
#include <atomic>

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

class EventLoop;
class Socket;
class Channel;


/*
设计原则：一个 TcpConnection 对象永远只由一个 I/O 线程（loop_）来操作。

所以所有的可能由应用层调用的对 TcpConnection 由影响的操作都是将对应的操作
放在TcpConnection 对应的loop的任务队列中由loop所在的线程串行执行
*/

/**
 * TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
 * => TcpConnection设置回调 => 设置到Channel => Poller => Channel回调
 **/
class TcpConnection :noncopyable ,public std::enable_shared_from_this<TcpConnection>
{
private:
    enum StateE
    {
        kConnected, //已经建立连接
        kConnecting, //正在建立连接
        kDisConnected, //已经断开连接
        kDisConnecting //正在断开连接
    };

    std::atomic_int state_;//这个连接所处的状态
    bool reading_;//连接是否在监听读事件


    //一个fd对应一个连接，也对应一个loop来进行事件的监听，所以一个连接对应一个loop
    EventLoop* loop_;//这个连接所在的loop

    // Socket Channel 这里和Acceptor类似    Acceptor => mainloop    TcpConnection => subloop
    std::unique_ptr<Socket>socket_;
    std::unique_ptr<Channel>channel_;

    const InetAddress local_addr_;//本机地址
    const InetAddress peer_addr_;//这个连接中对方的地址

    Buffer input_buffer_;//输入缓冲区
    Buffer output_buffer_;//输出缓冲区 只有在这个缓冲区中有数据的时候才监听写事件

    /*
    在输出缓冲区中设置一个高水位阈值，在输出缓冲区中积累的
    数据达到这个阈值之后，就会执行对应的回调函数
    */
    size_t high_water_mark_;//高水位阈值


    ConnecitonCallback connection_callback_;//在连接建立，关闭，销毁的时候调用
    CloseCallback close_callback_;//在连接关闭的时候调用
    WriteCompleteCallback write_complete_callback_;//在数据完全发送出去时调用
    HighWaterMarkCallback high_water_mark_callback_;//输出缓冲区达到阈值之后调用
    MessageCallback message_callback_;//在输入缓冲区由数据到来时调用

    const std::string name_;


    void setState(StateE state){state_=state;}//设置状态

    void handleRead(Timestamp receive_time); //读回调函数
    void handleWrite(); //写回调函数
    void handleClose(); //关闭回调函数
    void handleError(); //错误处理回调函数

    void shutDownInLoop();//具体执行
    void sendInLoop(const void* data,size_t len);//具体执行发送数据的函数
    void sendFileInLoop(int file_descriptor,off_t offset,size_t count);//具体执行发送文件的函数



public:
    constexpr static size_t DefaultWaterMark = 64 * 1024 * 1024; //默认为64mb

    TcpConnection(EventLoop* loop,
                const std::string&name,
                int sockfd,
                const InetAddress& local_addr,
                const InetAddress& peer_addr,
                size_t water_mark=DefaultWaterMark);
    ~TcpConnection();

    EventLoop* getLoop()const {return loop_;}
    const std::string& name()const {return name_;}
    const InetAddress& localAddress()const {return local_addr_;}
    const InetAddress& peerAddress()const {return peer_addr_;}

    bool connected(){return state_==kConnected;}

    void shutdown();//半关闭连接
    void send(const std::string& data);//发送数据
    void sendFile(int file_descriptor,off_t offset,size_t count);//发送文件

    void setConnecitonCallback(const ConnecitonCallback&cb){connection_callback_=cb;}
    void setCloseCallback(const CloseCallback&cb){close_callback_=cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback&cb){write_complete_callback_=cb;}
    void setHighWaterMarkCallback(const HighWaterMarkCallback&cb){high_water_mark_callback_=cb;}
    void setMessageCallback(const MessageCallback&cb){message_callback_=cb;}

    //连接建立
    void connectionEstablished();
    //连接销毁
    void connectionDestroyed();


    //测试使用
    int getState()
    {
        return state_.load();
    }

};


