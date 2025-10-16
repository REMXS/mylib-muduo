#include<stdlib.h>

#include"Poller.h"

//静态工厂函数的实现
Poller* Poller::newDefaultPoller(EventLoop*loop)
{
    if(::getenv("MUDO_POLL"))
    {
        return nullptr;
    }
    else
    {
        return nullptr;
    }
}