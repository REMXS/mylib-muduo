#include<arpa/inet.h>
#include<errno.h>

#include"Acceptor.h"
#include"Logger.h"
#include"Timestamp.h"
#include"InetAddress.h"


//创建一个非阻塞的fd
static int createNonBlocking()
{
    int fd=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,IPPROTO_TCP);
    if(fd<0)
    {
        LOG_FATAL("%s:%s:%d listen socket create error",__FILE__,__FUNCTION__,__LINE__)
    }
    return fd;   
}


Acceptor::Acceptor(EventLoop*loop,const InetAddress&addr,bool reuse)
    :loop_(loop)
    ,listening_(false)
    ,accept_socket_(createNonBlocking())
    ,accept_channel_(loop,accept_socket_.fd())
{
    accept_socket_.setReuseAddr(true);
    accept_socket_.setReusePort(reuse);
    accept_socket_.bindAddress(addr);
    accept_channel_.setReadCallback([this](Timestamp){
        this->handleRead();
    });
}


Acceptor::~Acceptor()
{
    accept_channel_.disableAll();
    accept_channel_.remove();
}

void Acceptor::listen()
{
    listening_=true;
    accept_socket_.listen();
    accept_channel_.enableReading();//要使用enableReading listenfd才能加入poller被监听读事件
}


void Acceptor::handleRead()
{
    InetAddress addr;
    int connfd=accept_socket_.accept(addr);

    if(connfd>0)
    {
        if(connection_callback_)
        {
            connection_callback_(connfd,addr);
        }
        else
        {
            ::close(connfd);//如果没有对新连接处理的函数，则关闭新的连接
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept error:%d",__FILE__,__FUNCTION__,__LINE__,errno);
        if(errno==EMFILE)
        {
            LOG_ERROR("%s:%s:%d socket fd reached limit",__FILE__,__FUNCTION__,__LINE__)
        }
    }
}
