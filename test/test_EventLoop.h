#pragma once

#include <thread>
#include <chrono>
#include <unistd.h>

#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"

void testEventloop() {
    EventLoop loop;
    int fds[2];
    if (::pipe(fds) != 0) {
        LOG_FATAL("pipe create failed");
    }

    Channel channel(&loop, fds[0]);
    channel.setReadCallback([&](Timestamp) {
        char buf[16] = {0};
        ::read(fds[0], buf, sizeof(buf));
        LOG_INFO("read callback triggered: %s", buf);
        loop.quit();
    });
    channel.enableReading();

    loop.queueInLoop([] {
        LOG_INFO("pending functor executed");
    });

    std::thread writer([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const char msg[] = "ping";
        ::write(fds[1], msg, sizeof(msg));
    });

    loop.loop();

    writer.join();
    ::close(fds[0]);
    ::close(fds[1]);

}