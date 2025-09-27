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

    static Timestamp now();

    std::string to_string() const;

private:
    int64_t microSecondsSinceEpoch_ ;
};