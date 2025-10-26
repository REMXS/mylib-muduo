#pragma once

#include<iostream>
#include<string>
#include<arpa/inet.h>
#include<unistd.h>


#include<gtest/gtest.h>

#include"Buffer.h"

TEST(BufferTest,ReadTest)
{
    Buffer buf(8);
    
    int fds[2];
    int pipe_err=::pipe(fds);
    ASSERT_EQ(pipe_err,0);

    ASSERT_EQ(buf.readableBytes(),0);
    ASSERT_EQ(buf.writeableBytes(),8);
    std::string data="hello world";
    write(fds[1],data.c_str(),data.size());

    int err=0;
    int n=buf.readFd(fds[0],err);
    ASSERT_EQ(n,data.size());
    ASSERT_EQ(std::string(buf.peek(),buf.readableBytes()),data);

    //读取数据
    std::string read_data=buf.retrieveAllAsString();
    ASSERT_EQ(read_data,data);
    
    //测试剩余空间大小
    ASSERT_EQ(buf.prependableBytes(),8);
    ASSERT_EQ(buf.writeableBytes(),data.size());


    //再次写入，触发移动式扩容操作
    buf.retrieve(5);
    write(fds[1],data.c_str(),data.size());
    n=buf.readFd(fds[0],err);
    ASSERT_EQ(buf.prependableBytes(),8);
    ASSERT_EQ(buf.readableBytes(),data.size());
    
    
    std::string test_s=R"(dfjlsjfl \n dfdsnefeif nv f lsjfd \t dnflsnf ,f lfj 7i932894 \0 29838&*(^*^*(%^&**T*WFT*(#@GGFDIWGB)#@($G@(YGIWE  \r\n FHO@#)))))";
    write(fds[1],test_s.c_str(),test_s.size());
    buf.readFd(fds[0],err);
    ASSERT_EQ(std::string(buf.peek(),buf.readableBytes()),data+test_s);


    ::close(fds[0]);
    ::close(fds[1]);
}


TEST(BufferTest,FullTest)
{
    Buffer read_buf(8);
    Buffer write_buf(8);

    int fds[2];
    pipe(fds);

    std::string data="hello world";
    write_buf.append(data.c_str(),data.size());

    ASSERT_EQ(write_buf.readableBytes(),data.size());
    ASSERT_EQ(write_buf.writeableBytes(),0);


    //写入数据
    int err=0;
    int n=write_buf.writeFd(fds[1],err);
    ASSERT_TRUE(n);
    write_buf.retrieve(n);
    ASSERT_EQ(write_buf.readableBytes(),0);
    ASSERT_EQ(write_buf.writeableBytes(),data.size());

    //读取数据
    read_buf.readFd(fds[0],err);
    std::string read_data=read_buf.retrieveAllAsString();
    ASSERT_EQ(read_data,data);
    
    //测试剩余空间大小
    ASSERT_EQ(read_buf.prependableBytes(),8);
    ASSERT_EQ(read_buf.writeableBytes(),data.size());


    ::close(fds[0]);
    ::close(fds[1]);
}
