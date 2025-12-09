#pragma once

#include <string>
#include <gtest/gtest.h>
#include <signal.h>
#include <chrono>
#include <thread>
#include <vector>
#include <arpa/inet.h>
#include <cassert>
#include <unistd.h>

#include "TcpServer.h"
#include "Logger.h"

//wsl default net.ipv4.ip_local_port_range = 44620    48715  
EventLoop* baseloop_sp=nullptr;
void signalHandler(int sig)
{
    if (sig == SIGINT)
    {
        LOG_INFO("Signal %d (SIGINT) received, shutting down...", sig);
        if(baseloop_sp)
        {
            baseloop_sp->quit();
        }
    }
}

void onConnection(const TcpConnectionPtr&conn)
{
    if(conn->connected())
    {
        LOG_INFO("%s connection established name=%s",__FUNCTION__,conn->name().c_str())
    }
    else
    {
        LOG_INFO("%s connection destroyed name=%s",__FUNCTION__,conn->name().c_str())
    }
}
void onMessage(const TcpConnectionPtr&conn,Buffer*buf,Timestamp receive_time)
{
    std::string data=buf->retrieveAllAsString();
    conn->send(data);
}

class IntergrationTest: public ::testing::Test
{
protected:
    const uint16_t test_port=9999;

    void SetUp()override
    {
        InetAddress addr(test_port);
        server_=std::make_unique<TcpServer>(&baseloop,addr,"server1");
        server_->setConnecitonCallback(onConnection);
        server_->setMessageCallback(onMessage);
        server_->setThreadNum(16);

        server_->start();
    }
    void TearDown()override
    {
        
    }

    
    EventLoop baseloop;
    std::unique_ptr<TcpServer>server_;
};

void stressTest()
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    for(int num=0;num<30;++num)
    {
        std::vector<int>sock_fds(1000,0);
        //创建连接
        for(auto&n:sock_fds)
        {
            n=socket(AF_INET,SOCK_STREAM,0);
            assert(n&&"failed to create a socket");
            int optval=1;
            ::setsockopt(n,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
            ::setsockopt(n,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof(optval));
        }
        //绑定地址，发送信息
        sockaddr_in addr;
        socklen_t addr_len=sizeof(addr);
        addr.sin_family=AF_INET;
        addr.sin_addr.s_addr=inet_addr("127.0.0.1");
        addr.sin_port=htons(9999);
        std::string data="hello world";
        for(auto&n:sock_fds)
        {
            int ret=connect(n,(sockaddr*)&addr,addr_len);
            if(ret==-1) perror("connect failed");
            assert(ret>=0&&"connect failed");
            write(n,data.c_str(),data.size());
        }

        //读取数据，验证数据的正确性
        for(auto&n:sock_fds)
        {
            char buf[32]={0};
            read(n,buf,32);
            ASSERT_EQ(data,buf);
        }

        //关闭连接
        for(auto&n:sock_fds)
        {
            ::close(n);
        }
    }
}

TEST_F(IntergrationTest,intergration_test)
{
    ::signal(SIGINT,signalHandler);

    std::thread t(stressTest);

    baseloop_sp=&baseloop;
    baseloop.loop();
    t.join();
}

