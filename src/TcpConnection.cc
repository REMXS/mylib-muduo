#include <functional>
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"


#include"TcpConnection.h"

static EventLoop* checkLoopNotNull(EventLoop* loop)
{
    if(loop==nullptr)
    {
        LOG_FATAL("%s:%s:%d main loop is null!",__FILE__,__FUNCTION__,__LINE__)
    }
    return loop;
}


TcpConnection::TcpConnection(EventLoop* loop,
            const std::string&name,
            int sockfd,
            const InetAddress& local_addr,
            const InetAddress& peer_addr,
            size_t water_mark)
    :state_(kConnecting)
    ,reading_(false)
    ,loop_(loop)
    ,socket_(std::make_unique<Socket>(sockfd))
    ,channel_(std::make_unique<Channel>(loop,sockfd))
    ,local_addr_(local_addr)
    ,peer_addr_(peer_addr)
    ,high_water_mark_(water_mark)
    ,sending_file(false)
{
    channel_->setCloseCallback([this](){handleClose();});
    channel_->setErrorCallback([this](){handleError();});
    channel_->setReadCallback([this](Timestamp receive_time){handleRead(receive_time);});
    channel_->setWriteCallback([this](){handleWrite();});

    LOG_INFO("new TCP connection created, name=%s, fd=%d",name_.c_str(),sockfd)

    socket_->setKeepAlive(true);
}


TcpConnection::~TcpConnection()
{
    LOG_INFO("TCP connection destroyed, name=%s, fd=%d, state=%d",name_.c_str(),socket_->fd(),state_.load())
}



void TcpConnection::shutdown()
{
    if(state_==kConnected)
    {
        setState(kDisConnecting);
        /*
        在对应的loop的doing pending functor 阶段中半关闭连接，
        避免在channel执行写回调时同时关闭写端导致错误 
        */
        loop_->runInLoop([this](){shutDownInLoop();});
    }
}

void TcpConnection::send(const std::string& data)
{
    if(state_==kConnected)
    {
        /*
        如果是loop是在当前线程中，比如在回调函数中执行心跳回复
        等简单的纯内存操作的时候，就会触发这个条件，从而使延迟降到
        最低，如果不进行条件判断直接加到loop的任务队列中就有点得不偿失了
        */
        if(loop_->isInLoopThread())
        {
            sendInLoop(data.c_str(),data.size());
        }
        else
        {
            loop_->runInLoop([this,&data](){sendInLoop(data.c_str(),data.size());});
        }
    }
    else
    {
        LOG_ERROR("%s not connected",__FUNCTION__)
    }
}

void TcpConnection::sendInLoop(const void* data,size_t len)
{
    ssize_t nwrote=0; //发送字节数
    size_t remaining=len; //剩余字节数
    bool fault_error=false; 

    //状态为kDisConnected表明调用过shutDownWrite关闭了写端或者是连接即将关闭，不能再进行发送了
    if(state_==kDisConnected)
    {
        LOG_ERROR("disconnected, give up writing")
    }

    //如果channel的写事件没有被监听并且输出缓冲区中没有数据，则可以直接发送数据
    if(!channel_->isWriting()&&output_buffer_.readableBytes()==0)
    {
        nwrote=::write(channel_->fd(),data,len);

        if(nwrote>=0)
        {
            remaining-=nwrote;

            //如果全部数据发送完毕，则执行响应的回调函数
            if(remaining==0&&write_complete_callback_)
            {
                loop_->queueInLoop([this](){write_complete_callback_(shared_from_this());});
            }
        }
        else
        {
            nwrote=0;
            if(errno!=EWOULDBLOCK)
            {
                LOG_ERROR("%s error happend",__FUNCTION__)

                if(errno==EPIPE||errno==ECONNRESET) // sigpipe received, connection reset
                {
                    fault_error=true;
                }
            }
        }
    }

    /*
    如果有剩余的数据没有发送或者是在输出缓冲区中还有数据，则在输出缓冲区中追加数据
    如果原先输出缓冲区中没有数据，启用channel的写事件监听（向poller注册写事件，然后等待poller返回EPOLLOUT）
    等poller返回epollout之后，执行hanlewrite回调函数，将输出缓冲区中剩余的数据发送出去
    */
    if(!fault_error&&remaining)
    {
        size_t old_len=output_buffer_.readableBytes();
        //在追加之前要判断输出缓冲区中的数据是否超过high water mark，决定是否要执行响应的回调函数
        if(old_len+remaining>=high_water_mark_&&old_len<high_water_mark_&&high_water_mark_callback_)
        {
            loop_->queueInLoop([this](){high_water_mark_callback_(shared_from_this());});
        }

        output_buffer_.append((char*)data+nwrote,remaining);

        //如果之前输出缓冲区中没有数据，现在有数据了，启用poller监听写事件
        if(!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}


void TcpConnection::sendFile(int file_descriptor,off_t offset,size_t count)
{
    if(state_==kConnected)
    {
        if(loop_->isInLoopThread())
        {
            sendFileInLoop(file_descriptor,offset,count);
        }
        else
        {
            loop_->queueInLoop([file_descriptor,offset,count,this](){sendFileInLoop(file_descriptor,offset,count);});
        }
    }
    else
    {
        LOG_ERROR("%s fd=%d not connect",__FUNCTION__,channel_->fd())
    }
}



void TcpConnection::sendFileInLoop(int file_descriptor,off_t offset,size_t count)
{
    if(state_==kDisConnected||state_==kDisConnecting)
    {
        LOG_ERROR("%s connection fd=%d is down, give up sending",__FUNCTION__,socket_->fd())
    }

    ssize_t bytes_send=0;






}


void TcpConnection::connectionEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this()); //绑定到TCP Connection对象
    channel_->enableReading(); //向eventloop中的poller注册读监听事件

    //连接建立，执行回调
    if(connection_callback_)
    {
        /* 
        根据 C++ 标准：绑定到 const 引用的临时对象，其生命周期会被延长到该引用的作用域结束。
        也就是一个右值可以绑定到一个const修饰的左值引用的变量上,下面也有类似的操作
        */
        connection_callback_(shared_from_this());
    }
}

void TcpConnection::connectionDestroyed()
{
    if(state_==kConnected||state_==kDisConnecting)
    {
        setState(kDisConnected);
        channel_->disableAll(); //将channel在poller中注册的监听事件都取消

        //连接销毁，执行回调
        if(connection_callback_)
        {
            connection_callback_(shared_from_this());
        }
    }

    channel_->remove(); //把channel从poller中删除
}


void TcpConnection::handleRead(Timestamp receive_time)
{
    int saved_err=0;
    ssize_t n=input_buffer_.readFd(channel_->fd(),saved_err);

    if(n>0)
    {
        if(message_callback_)
        {
            //执行应用层设置的回调函数来读取处理数据
            //处于性能考虑，这里没有将回调函数加入到loop中的任务队列中执行
            message_callback_(shared_from_this(),&input_buffer_,receive_time);
        }
    }
    else if(n==0)//连接关闭
    {
        handleClose();
    }
    else//读取出错
    {
        errno=saved_err;
        LOG_ERROR("%s fd=%d read failed",__FUNCTION__,channel_->fd());
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if(channel_->isWriting())
    {
        int saved_err=0;
        ssize_t n=output_buffer_.writeFd(channel_->fd(),saved_err);
        if(n>0)
        {
            output_buffer_.retrieve(n);
           
            if(output_buffer_.readableBytes()==0)
            {
                //如果全部写入数据，停止poller监听写事件
                channel_->disableWriting();
                if(write_complete_callback_)
                {
                    /*
                    如果输出缓冲区全部写入fd的写缓冲区，则触发回调函数
                    在此函数中负责的责任应该是读取数据，后续的其它操作应该
                    在loop 的 doing pending functor 阶段完成 
                    */
                    loop_->queueInLoop([this](){write_complete_callback_(shared_from_this());});
                }
            }

            //如果在输出缓冲区发送完之前就主动停止连接，则关闭写端
            if(state_==kDisConnecting)
            {
                shutDownInLoop();
            }
        }
        else
        {
            LOG_ERROR("%s connection fd=%d write error:%d",__FUNCTION__,channel_->fd(),saved_err)
        }
    }
    else
    {
        LOG_ERROR("%s connection fd=%d is down, no more writing",__FUNCTION__,channel_->fd())
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("%s connection fd=%d closed",__FUNCTION__,channel_->fd())
    setState(kDisConnected);
    channel_->disableAll();

    TcpConnectionPtr connfd_ptr=shared_from_this();
    //连接关闭，执行回调
    if(connection_callback_)
    {
        connection_callback_(connfd_ptr);
    }
    if(close_callback_)
    {
        close_callback_(connfd_ptr);
    }
}

void TcpConnection::handleError()
{
    int optval=0;
    socklen_t optlen=sizeof(optval);
    int err=0;

    if(::getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen)==0)
    {
        err=optval;
    }
    else
    {
        err=errno;
    }
    LOG_ERROR("%s connection fd=%d name=%s error happen ,errno=%d",__FUNCTION__,channel_->fd(),name_.c_str(),err)
}

void TcpConnection::shutDownInLoop()
{
    //如果poller没有监听channel的写事件，则说明输出缓冲区中没有数据要发送，可以关闭fd的写端
    if(!channel_->isWriting())
    {
        socket_->shutDownWrite();
    }
}


