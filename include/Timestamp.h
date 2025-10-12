#pragma once

#include<iostream>
#include<string>

class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    Timestamp(const Timestamp& ts);

    Timestamp& operator=(int64_t microSecondsSinceEpoch);
    Timestamp& operator=(const Timestamp& ts);


    //获取当前时间
    static Timestamp now();

    //获取当前时间的字符串类型
    std::string to_string() const;

private:
    int64_t microSecondsSinceEpoch_ ;
};