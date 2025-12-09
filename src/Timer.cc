#include "Timer.h"

void Timer::restart()
{
    if(repeat_)
    {
        expiration_ = addTime(expiration_,interval_);
    }
    else
    {
        expiration_ = Timestamp::invaild();
    }
}
