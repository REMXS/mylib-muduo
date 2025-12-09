#pragma once

#include <thread>
#include <chrono>
#include <unistd.h>
#include <string>
#include <atomic>
#include <gtest/gtest.h>

#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"


TEST(EventLoopTest, BasicEventLoop)
{
    EventLoop loop;
    int fds[2];
    if (::pipe(fds) != 0) {
        LOG_FATAL("pipe create failed");
    }
    std::string content = "hello";

    std::atomic_int pending_cnt;

    Channel channel(&loop, fds[0]);
    channel.setReadCallback([&](Timestamp) {
        std::string buf(16,0);
        ::read(fds[0], buf.data(), buf.size());
        EXPECT_EQ(buf.substr(0, content.size()), content);
        loop.quit();
    });
    channel.enableReading();

    loop.queueInLoop([&pending_cnt] {
        ++pending_cnt;
    });

    std::thread writer([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ::write(fds[1], content.data(), content.size());
    });

    loop.loop();

    writer.join();
    EXPECT_EQ(pending_cnt.load(), 1);
    ::close(fds[0]);
    ::close(fds[1]);
}