#include<iostream>
#include"test_Timestamp.h"
#include"test_Logger.h"
#include"test_CurrentThread.h"

// #if defined(__linux__)
//     #include<sys/epoll.h>
// #elif defined(__APPLE__)
//     #include<sys/sysctl.h>
// #endif

int main(){
    //test_Timestamp();
    //test_logger();
    test_CurrentThread();
    return 0;
}