#include<iostream>
#if defined(__linux__)
    #include<sys/epoll.h>
#elif defined(__APPLE__)
    #include<sys/sysctl.h>
#endif

int main(){

    return 0;
}