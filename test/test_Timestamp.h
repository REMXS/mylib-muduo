#pragma once

#include<iostream>
#include"Timestamp.h"

// #if defined(__linux__)
//     #include<sys/epoll.h>
// #elif defined(__APPLE__)
//     #include<sys/sysctl.h>
// #endif

void test_Timestamp(){
    std::cout<<"start test Timestamp"<<std::endl;
    Timestamp ts;
    ts=Timestamp::now();
    Timestamp ts2(ts);
    Timestamp ts3(time(NULL));

    Timestamp ts4=ts3;
    ts4=ts3;
    std::cout<<ts.to_string()<<std::endl;
    std::cout<<ts2.to_string()<<std::endl;
    std::cout<<ts3.to_string()<<std::endl;
    std::cout<<ts4.to_string()<<std::endl;


}