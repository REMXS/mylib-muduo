## 项目介绍

本项目基于c++ 17 ，仿照muduo-core的高性能多线程非阻塞的网络库
实现了一个基于主从多reactor的并发模型
封装了Linux epoll I/O 多路复用机制，为上层用户提供易用，线程安全的tcp服务器开发接口

## 核心并发模型
### 主reactor
#### 职责
监听listenfd，通过Acceptor模块接收新的连接
#### 工作流
当listenfd 有事件可读时，拥有listenfd的epoll返回，执行读回调函数，通过轮询算法选择一个subloop，建立连接
### 从reactor
#### 职责
监听tcp连接发生的事件，通过channel模块的回调函数处理各种epoll返回的事件
#### 工作流
当有事件发生时，epoll返回发生事件的epoll_event集合，先转换为channel的集合，然后根据返回的事件执行对应的回调函数
### 优势
1. mainloop的职责单一，响应新连接的速度极快
2. 所有的I/O读写都分散到不同的reactor中，分散了I/O的压力
3. one loop per thread ，唯一的共享资源是每个loop的任务队列，持有锁的时间很短，通知使用了eventfd来唤醒线程中阻塞的epoll，执行响应的任务，并且每一个线程中的资源由每一个线程所有，其它线程想要修改这个线程中的资源，要在这个线程中的任务队列中加入任务，最后由这个线程串行执行修改资源的操作，避免了加锁带来的效率下降和潜在的多线程并发问题。每个 TcpConnection 的所有操作都在同一个线程内完成，无需对 Buffer 或连接状态加锁，极大降低了并发编程的复杂度。

## 模块化架构解析
### reactor核心（事件驱动）
* EventLoop: reactor 模式的核心，封装了时间循环，拥有poller 和 channel 列表，主要循环的流程为先调用poller获取发生事件的channel列表，然后执行事件回调，在执行追加到任务队列中的数据
* Poller/EpollPoller: 主要负责单次循环中的事件监听，Poller为抽象基类，EpollPoller为具体的实现类，使用 epoll_wait 获取发生事件的fd，然后转换为channel的列表返回给EventLoop
* Channel: 连接TCPConnection与 EventLoop，TCPConnection中的fd封装在channel像EventLoop中注册从而监听连接中的各种事件，内部封装了TCPConnection的fd和连接感兴趣的事件以及各个事件的回调函数，每次EventLoop获取到channel的列表，然后根据每一个channel发生的事件执行对应的回调函数
## 服务器封装（逻辑组装）
* TcpServer:组合了Acceptor和EventLoopThreadPool，负责启动服务器（start()），和管理连接的生命周期，。
* Acceptor: 作用相当于一个“接待员”，运行在mainloop中，像是是一个特殊的TCPConnection，封装了用于监听的fd和其用于连接建立和分发的回调函数，在mainloop中有读事件发生时，调用Acceptor内部channel中的回调函数用来接收新的连接并将其分发到EventLoop中进行监听
* TCPConnection: 作用相当于一个“服务员”，运行在子线程中的EventLoop中，主线程中的Acceptor接收连接时建立TCPConnection，然后将其分发到其它EventLoop中（向其任务队列中添加启动事件）进行监听，由对应的EventLoop进行管理
## 线程模型
* EventLoopThreadPool: subloop的线程池，管理若干的子线程，并通过轮询的方式返回子线程中的eventloop对象，用于主线程中eventloop分发连接
* EventLoopThread: 封装了EventLoop对象和Thread对象，内部有执行EventLoop循环的函数，这个函数在内部创建EventLoop对象，然后将对象指针赋值到EventLoopThread的成员变量，Thread对象执行这个函数
* Thread: 封装c++ 的thread对象，在thread对象中执行给定的函数，将执行子线程分为了配置和启动（start()）两个阶段，并提供了获取tid，自定义线程的名称等功能
## 基础工具（c++ 封装）
* Socket/InetAddress: 对socket C-API 的RAII封装，封装了对socket的一些操作和设置，对象析构时自动执行::close(fd),防止文件描述符泄露，InetAddress是对sockaddr_in 的封装，方便获取ip地址和端口号
* Buffer: 非阻塞I/O的核心，作为TCPConnection的读写缓冲区，解决了read数据不完整和write写数据一次性写不完的问题，
* CurrentThread: 使用 thread_local 变量存储tid，避免了频繁的系统调用
* noncopyable: 工具基类，通过 C++ delete 特性防止核心类（如 EventLoop）被拷贝
