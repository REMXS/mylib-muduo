#include<signal.h>

// #include "test_Timestamp.h"
// #include "test_Thread.h"
// #include "test_Logger.h"
// #include "test_EventLoop.h"
// #include "test_CurrentThread.h"
// #include "test_Channel.h"

//#include"test_TcpConnection.h"
//#include "Intergration_test.h"

#include "test_Timer.h"


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