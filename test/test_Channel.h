#pragma once

#include"Channel.h"
#include"EventLoop.h"
#include"Timestamp.h"

#include<iostream>
#include<string>
#include<sys/epoll.h>
#include<atomic>
#include<gtest/gtest.h>



TEST(ChannelTest, Basic)
{
    EventLoop* ep=nullptr;
    Channel c(ep,12);

    EXPECT_EQ(c.fd(),12);

    std::shared_ptr<std::string>s=std::make_shared<std::string>("hel");
    

    //回调函数计数器
    std::atomic_int read_callback_count{0};
    std::atomic_int callback_count{0};

    auto close_callback=[&](){
        std::cout<<"this is close callback"<<std::endl;
        callback_count++;
    };
    auto read_callback=[&](Timestamp){
        std::cout<<"this is read callback"<<std::endl;
        read_callback_count++;
    };
    auto write_callback=[&](){
        std::cout<<"this is write callback"<<std::endl;
        callback_count++;
    };
    auto error_callback=[&](){
        std::cout<<"this is error callback"<<std::endl;
        callback_count++;
    };


    c.setCloseCallback(close_callback);
    c.setErrorCallback(error_callback);
    c.setReadCallback(read_callback);
    c.setWriteCallback(write_callback);
    c.handleEvent(Timestamp::now());


    c.tie(s);
    c.setIndex(1);
    EXPECT_EQ(c.index(),1);
    EXPECT_EQ(callback_count.load(),0);

    c.set_revent(EPOLLIN);
    c.handleEvent(Timestamp::now());

    c.set_revent(EPOLLOUT);
    c.handleEvent(Timestamp::now());

    c.set_revent(EPOLLPRI);
    c.handleEvent(Timestamp::now());

    c.set_revent(EPOLLHUP|EPOLLIN);
    c.handleEvent(Timestamp::now());
    
    c.set_revent(EPOLLHUP);
    c.handleEvent(Timestamp::now());

    c.set_revent(EPOLLERR);
    c.handleEvent(Timestamp::now());

    //判断计数器是否正确
    EXPECT_EQ(callback_count.load(),3);
    EXPECT_EQ(read_callback_count.load(),3);
    
    s.reset();
    //在上层对象销毁后调用handleEvent
    c.set_revent(EPOLLIN);
    c.handleEvent(Timestamp::now());
    
    EXPECT_EQ(callback_count.load(),3);
    EXPECT_EQ(read_callback_count.load(),3);
}