#include "Timestamp.h"
#include<time.h>

Timestamp::Timestamp():microSecondsSinceEpoch_(0)
{

}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch):microSecondsSinceEpoch_(microSecondsSinceEpoch)
{

}

Timestamp::Timestamp(const Timestamp& ts):microSecondsSinceEpoch_(ts.microSecondsSinceEpoch_)
{

}


Timestamp& Timestamp::operator=(const int64_t microSecondsSinceEpoch)
{
    this->microSecondsSinceEpoch_=microSecondsSinceEpoch;
    return *this;
}

Timestamp& Timestamp::operator=(const Timestamp& ts)
{
    this->microSecondsSinceEpoch_=ts.microSecondsSinceEpoch_;
    return *this;
}

Timestamp Timestamp::now()
{
    return Timestamp(time(NULL));
}

std::string Timestamp::to_string() const
{
    char buf[128]={0};
    tm* t= localtime(&this->microSecondsSinceEpoch_);
    //%02d 表示不足时用0填充
    snprintf(buf,128,"%4d/%02d/%02d %02d/%02d/%02d",
        t->tm_year+1900,
        t->tm_mon+1,
        t->tm_mday,
        t->tm_hour,
        t->tm_min,
        t->tm_sec
    );
    return buf;
}