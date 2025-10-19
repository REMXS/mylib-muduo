#include<iostream>
#include<gtest/gtest.h>

#include"test_Thread.h"

// #if defined(__linux__)
//     #include<sys/epoll.h>
// #elif defined(__APPLE__)
//     #include<sys/sysctl.h>
// #endif


int main(int argc,char**argv){
    test_thread();
    ::testing::InitGoogleTest(&argc,argv);


    return RUN_ALL_TESTS();
}