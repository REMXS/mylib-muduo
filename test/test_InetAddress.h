#pragma once

#include "gtest/gtest.h"
#include"InetAddress.h"

TEST(InetAddressTest,BaseTest)
{
    InetAddress ia("127.0.0.2",9999);
    InetAddress ia2=ia;
    
    auto addr=ia.getSockAddr();
    ASSERT_NE(addr,nullptr);
    ASSERT_EQ(ia2.toIp(),"127.0.0.2");
    ASSERT_EQ(ia2.toPort(),9999);
    ASSERT_EQ(ia2.toIpPort(),"127.0.0.2:9999");
}