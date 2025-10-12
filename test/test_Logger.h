#pragma once

#include<iostream>
#include"Logger.h"

void test_logger(){
    LOG_INFO("hello,world")
    LOG_INFO("this is %d",199)
    LOG_INFO("%d,%s",123,"hel")
    LOG_ERROR("hello,world")
    LOG_ERROR("this is %d",199)
    LOG_ERROR("%d,%s",123,"hel")
    LOG_FATAL("hello,world")
    LOG_FATAL("this is %d",199)
    LOG_FATAL("%d,%s",123,"hel")
    LOG_DEBUG("hello,world")
    LOG_DEBUG("this is %d",199)
    LOG_DEBUG("%d,%s",123,"hel")
}