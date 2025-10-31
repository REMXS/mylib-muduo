#include<signal.h>

//#include"test_Thread.h"
//#include"test_EventLoopThreadPool.h"

//#include"test_InetAddress.h"
//#include"test_Socket.h"
//#include"test_Buffer.h"

#include"test_TcpConnection.h"
// #if defined(__linux__)
//     #include<sys/epoll.h>
// #elif defined(__APPLE__)
//     #include<sys/sysctl.h>
// #endif


int main(int argc,char**argv){
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;  // 忽略
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    ::testing::InitGoogleTest(&argc,argv);

    
    return RUN_ALL_TESTS();
}