#pragma once

#include <unordered_map>
#include <memory>

#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "TcpConnection.h"

class TcpServer
{
private:
    using ConnectionMap=std::unordered_map<std::string,TcpConnectionPtr>;

    EventLoop *loop_; //EventLoopThreadPool 中的baseloop(也就是mainloop)
    std::string ip_port; //ip和端口
    std::string name_; //tcp server 的名字
    std::unique_ptr<Acceptor> acceptor_; //在mainloop中用于监听建立连接的fd

    std::shared_ptr<EventLoopThreadPool>pool_; //reactor线程池 ， one loop per thread

    
    ConnectionMap connection_map_; //保存所有连接的容器

    ConnecitonCallback connection_callback_; //当连接建立和停止时所调用的回调函数
    WriteCompleteCallback write_complete_callback_; //当数据完全发送完毕时执行的回调函数
    MessageCallback message_callback_; //当数据到来时执行的回调函数

    EventLoopThreadPool::ThreadInitCallback thread_init_callback_; //线程初始化时调用的回调函数
    int num_threads_; //线程池中子线程的数量，也就是subloop的数量

    std::atomic_int started_; //表示tcpserver是否开启

    int next_conn_id_; //一个简单的计数器，用于生成下一个tcp连接的id


    void newConnection(int sock_fd,const InetAddress&peer_addr);//用于建立新连接的函数，在acceptor中作为回调函数调用
    
    //移除连接
    void removeConnection(const TcpConnectionPtr& tcp_connection_ptr);
    void removeConnectionInLoop(const TcpConnectionPtr& tcp_connection_ptr);


public:
    enum Option
    {
        kNoReusePort,kReusePort
    };


    TcpServer(EventLoop* base_loop,
              InetAddress bind_addr,
              std::string name,
              Option reuse_option=kNoReusePort
              );

    ~TcpServer();

    void start();

    void setThreadNum(int thread_num); //设置线程池中的线程数量

    /*
    Pass-by-Value and Move 进行优化，
    这里如果时右值绑定到cb对象，直接调用移动构造函数，总共有两次移动，0拷贝
    这里如果是一个左值，则发生一次拷贝和移动，没有 move函数带来的问题
    */
    void setConnecitonCallback(ConnecitonCallback cb){connection_callback_=std::move(cb);}
    void setWriteCompleteCallback(WriteCompleteCallback cb){write_complete_callback_=std::move(cb);}
    void setMessageCallback(MessageCallback cb){message_callback_=std::move(cb);}

    void setThreadInitCallback(EventLoopThreadPool::ThreadInitCallback cb){thread_init_callback_=std::move(cb);}
};

