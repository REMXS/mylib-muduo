#pragma once
#include <gtest/gtest.h>
#include <memory>

#include "TimerQueue.h"
#include "EventLoop.h"
#include "Channel.h"
#include "TimerId.h"


class TimerQueueTest: public ::testing::Test
{
protected:
    std::unique_ptr<EventLoop> loop;
    std::unique_ptr<TimerQueue> timer_queue;

    void SetUp() override
    {
        loop = std::make_unique<EventLoop>();
        timer_queue = std::make_unique<TimerQueue>(loop.get());
    }

    void TearDown() override
    {
        timer_queue.reset();
        loop.reset();
    }
};

//测试基本定时器的添加和触发
TEST_F(TimerQueueTest, basic_timer)
{
    bool is_called=false;
    timer_queue->addTimer([&](){
        is_called=true;
        loop->quit();
    },addTime(MonotonicTimestamp::now(), 0.1),0);

    loop->loop();
    EXPECT_TRUE(is_called);
}

//测试重复定时器
TEST_F(TimerQueueTest, repeating_timer)
{
    int call_count=0;
    timer_queue->addTimer([&](){
        call_count++;
        if(call_count==3)
        {
            loop->quit();
        }
    },addTime(MonotonicTimestamp::now(), 0.1),0.1);

    loop->loop();
    EXPECT_EQ(call_count,3);
}

//测试取消定时器
TEST_F(TimerQueueTest, cancel_timer)
{
    bool should_not_be_called=false;
    bool should_be_called=false;
    TimerId timer_id = timer_queue->addTimer([&](){
        should_not_be_called=true;
    },addTime(MonotonicTimestamp::now(), 0.1),0);
    timer_queue->cancelTimer(timer_id);

    timer_queue->addTimer([&](){
        should_be_called=true;
        loop->quit();
    },addTime(MonotonicTimestamp::now(), 0.2),0);

    loop->loop();
    EXPECT_FALSE(should_not_be_called);
    EXPECT_TRUE(should_be_called);
}

//测试取消正在执行的定时器
TEST_F(TimerQueueTest, cancel_during_callback)
{
    //添加一个重复定时器，在其回调中取消自身
    int cnt=0;
    TimerId timer_id = timer_queue->addTimer([&](){
        cnt++;
        if(cnt==2)
        {
            timer_queue->cancelTimer(timer_id);
            timer_queue->addTimer([&](){
                loop->quit();
            },addTime(MonotonicTimestamp::now(), 0.1),0);
        }
    },addTime(MonotonicTimestamp::now(), 0.1),0.1);
    loop->loop();
    EXPECT_EQ(cnt,2);
}

//测试相同时间点添加多个定时器
TEST_F(TimerQueueTest, multiple_timers_same_time)
{
    int cnt=0;
    auto timer_task=[&](){
        cnt++;
    };

    TimerId timer_id1 = timer_queue->addTimer(timer_task,addTime(MonotonicTimestamp::now(), 0.1),0);
    timer_queue->addTimer(timer_task,addTime(MonotonicTimestamp::now(), 0.1),0);
    timer_queue->addTimer(timer_task,addTime(MonotonicTimestamp::now(), 0.1),0);
    timer_queue->addTimer(timer_task,addTime(MonotonicTimestamp::now(), 0.1),0);
    timer_queue->cancelTimer(timer_id1);

    timer_queue->addTimer([&](){
        loop->quit();
    },addTime(MonotonicTimestamp::now(), 0.2),0);

    loop->loop();
    EXPECT_EQ(cnt,3);
}




