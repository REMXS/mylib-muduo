#pragma once

#include"Channel.h"
#include"EventLoop.h"
#include"Timestamp.h"

#include<iostream>
#include<string>
#include<sys/epoll.h>


void close_callback(int a){
    std::cout<<a<<" "<<"this is close callback"<<std::endl;
}
void read_callback(Timestamp a){
    std::cout<<a.to_string()<<" "<<"this is read callback"<<std::endl;
}

void write_callback(int a){
    std::cout<<a<<" "<<"this is write callback"<<std::endl;
}

void error_callback(int a){
    std::cout<<a<<" "<<"this is error callback"<<std::endl;
}



void test_Channel()
{
    EventLoop* ep=nullptr;
    Channel c(ep,12);
    std::cout<<c.fd()<<" "<<c.events()<<std::endl;

    std::shared_ptr<std::string>s=std::make_shared<std::string>("hel");
    

    c.setCloseCallback(std::bind(close_callback,0));
    c.setErrorCallback(std::bind(error_callback,1));
    c.setReadCallback(std::bind(read_callback,Timestamp::now()));
    c.setWriteCallback(std::bind(write_callback,3));
    c.handleEvent(Timestamp::now());

    


    c.tie(s);
    c.setIndex(1);
    std::cout << ((c.index() == 1) ? "index is right" : "index is wrong") << std::endl;

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
    
    s.reset();
    std::cout<<"after destroy the obj, try to call the callback function..."<<std::endl;
    c.handleEvent(Timestamp::now());


}