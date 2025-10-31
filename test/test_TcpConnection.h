#pragma once

#include <memory>
#include <vector>
#include <set>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h> //获取文件大小
#include <signal.h>


#include "gtest/gtest.h"

#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "CurrentThread.h" 
#include"Logger.h"
#include"TcpConnection.h"
#include"InetAddress.h"
#include "Channel.h"


class TcpConnectionTest: public ::testing::Test
{   
protected:
    enum StateE
    {
        kConnected, //已经建立连接
        kConnecting, //正在建立连接
        kDisConnected, //已经断开连接
        kDisConnecting //正在断开连接
    };
    void SetUp() override
    {
        base_loop_=std::make_unique<EventLoop>();
        pool_=std::make_unique<EventLoopThreadPool>(base_loop_.get(),"poll");
        pool_->setNumThreads(1);

        //创建套接字 注意socketpair 虽然是模拟网络通信，但是本质上还是本地通信，不能用网络协议栈
        if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, sv) < 0) {
            perror("socketpair failed");
            return;
        }

        std::function<void(EventLoop*)>tem_fu;
        pool_->start(tem_fu);

        auto loops=pool_->getAllLoops();

        //注意：这里用socketpair创建的套接字自动建立连接，直接可以通信，所以这里的地址均没有实际作用
        conn=std::make_shared<TcpConnection>(loops[0],name,sv[0],local_addr,local_addr,8);
    }

    void TearDown() override
    {
        //关闭套接字
        ::close(sv[0]);
        ::close(sv[1]);
    }


    std::unique_ptr<EventLoop>base_loop_;
    std::unique_ptr<EventLoopThreadPool>pool_;
    std::string name="test";
    const InetAddress local_addr;

    //套接字
    int sv[2];

    //测试对象
    std::shared_ptr<TcpConnection>conn;
};


//测试连接的建立销毁的正确性
TEST_F(TcpConnectionTest,TestCreateDestroy)
{
    

    ASSERT_EQ(conn->getState(),kConnecting);
    conn->connectionEstablished();
    ASSERT_TRUE(conn->connected());
    conn->connectionDestroyed();
    ASSERT_FALSE(conn->connected());
    ASSERT_EQ(conn->getState(),kDisConnected);
}



//测试channel中callback函数的功能
TEST_F(TcpConnectionTest,TestChannelCallback)
{
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic_int cnt=0;
    int conn_cnt=0; //2
    int write_complete_cnt=0; //1
    int water_mark_cnt=0; //1
    int close_cnt=0; //1
    int message_cnt=0; //1

    int expect_cnt=7;

    std::string data="hello";

    //设置回调函数
    conn->setCloseCallback([&](const TcpConnectionPtr&){
        close_cnt++;
        cnt++;
        if(cnt==expect_cnt) cv.notify_one();
    });
    conn->setConnecitonCallback([&](const TcpConnectionPtr&){
        conn_cnt++;
        cnt++;
        if(cnt==expect_cnt) cv.notify_one();
    });
    conn->setHighWaterMarkCallback([&](const TcpConnectionPtr&){
        water_mark_cnt++;
        cnt++;
        if(cnt==expect_cnt) cv.notify_one();
    });
    conn->setMessageCallback([&](const TcpConnectionPtr&,Buffer*buf,Timestamp){
        ASSERT_EQ(buf->retrieveAllAsString(),data);
        message_cnt++;
        cnt++;
        if(cnt==expect_cnt) cv.notify_one();
    });
    conn->setWriteCompleteCallback([&](const TcpConnectionPtr&){
        write_complete_cnt++;
        cnt++;
        if(cnt==expect_cnt) cv.notify_one();
    });

    //正式建立连接
    conn->connectionEstablished();

    //发送小数据
    conn->send(data);

    //发送大数据
    std::string large_message(4096*64*4,'A');
    conn->send(large_message);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));


    //接收数据
    ::send(sv[1],data.c_str(),data.size(),0);

    //测试WriteCompleteCallback
    int n=0;
    int ncnt=0;
    char buf[4096]={0};
    while((n=::read(sv[1],buf,sizeof(buf)))!=-1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ncnt+=n;
    }
    if (n < 0 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
        perror("read error");
    }
    ASSERT_EQ(ncnt,large_message.size()+data.size());

    ::close(sv[1]);

    {
        std::unique_lock<std::mutex>lock(mtx);
        cv.wait(lock,[&](){
            return cnt==expect_cnt;
        });
    }

    conn->connectionDestroyed();
    
    ASSERT_EQ(conn_cnt,2);
    ASSERT_EQ(write_complete_cnt,2);
    ASSERT_EQ(water_mark_cnt,1);
    ASSERT_EQ(close_cnt,1);
    ASSERT_EQ(message_cnt,1);
    
}


TEST_F(TcpConnectionTest,TestShutDown)
{
    conn->shutdown();
    ASSERT_EQ(conn->getState(),kConnecting);

    conn->connectionEstablished();


    //发送大数据
    std::string large_message(4096*64*4,'A');
    conn->send(large_message);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));


    conn->shutdown();
    ASSERT_EQ(conn->getState(),kDisConnecting);

    conn->send("hello");

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));


    //测试WriteCompleteCallback
    int n=0;
    char buf[4096]={0};

    
    /*
    read 返回 0：对端已关闭写端或文件到达 EOF。
    read 返回 -1 且 errno=EAGAIN/EWOULDBLOCK：非阻塞模式下当前无数据可读，后续仍可再尝试。
    */
    //返回 0 表示对端已关闭写端，EOF 会一直返回 0，所以这里的while不能与上一个条件相同
    while((n=::read(sv[1],buf,sizeof(buf)))>0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (n == 0) {
        // EOF
    } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("read error");
    }


    conn->connectionDestroyed();
    ASSERT_EQ(conn->getState(),kDisConnected);
}


TEST_F(TcpConnectionTest,SendFileTest)
{
    const char* file_name="../test/WW_win95_v004_light_post.png";
    FILE* fp=fopen(file_name,"r");
    int fd=fileno(fp);
    
    struct stat st;
    int tem=::stat(file_name,&st);
    ASSERT_EQ(tem,0);

    off_t file_size=st.st_size;

    conn->connectionEstablished();
    //发送文件
    conn->sendFile(fd,0,static_cast<size_t>(file_size));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));


    //接收文件
    long n=0;
    long cnt=0;
    char buf[8192]={0};
    while((n=::read(sv[1],buf,sizeof(buf)))>0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        cnt+=n;
    }
    if (n == 0) {
        // EOF
    } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("read error");
    }

    ASSERT_EQ(cnt,file_size);

    fclose(fp);

    ASSERT_EQ(conn->getState(),kConnected);

    conn->connectionDestroyed();
}



