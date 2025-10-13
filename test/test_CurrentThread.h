#include"CurrentThread.h"
#include<iostream>
void test_CurrentThread(){
    std::cout<<CurrentThread::tid()<<std::endl;
}