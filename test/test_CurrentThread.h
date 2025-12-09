#pragma once
#include"CurrentThread.h"
#include<iostream>
#include<gtest/gtest.h>


TEST(CurrentThreadTest, Basic) {
    EXPECT_EQ(CurrentThread::tid(), CurrentThread::tid());
    std::cout << "CurrentThread ID: " << CurrentThread::tid() << std::endl;
}