

//#include"test_Thread.h"
//#include"test_EventLoopThreadPool.h"

//#include"test_InetAddress.h"
//#include"test_Socket.h"
#include"test_Buffer.h"


// #if defined(__linux__)
//     #include<sys/epoll.h>
// #elif defined(__APPLE__)
//     #include<sys/sysctl.h>
// #endif


int main(int argc,char**argv){
    ::testing::InitGoogleTest(&argc,argv);

    
    return RUN_ALL_TESTS();
}