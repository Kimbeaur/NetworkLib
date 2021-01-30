/**************************************************
 * @Description 
 * @Author kimbeaur
 * @Date 2021/1/30 09:42
 *************************************************/


#pragma once

#include <stdint.h>
#include <stdio.h> //NULL
/*
 * 定义IO复用机制事件的封装(epoll原生事件来封装)
 *
 * */

class EventLoop;

//IO事件触发的回调函数
typedef void IOCallback(EventLoop *loop, int fd, void *args);
//Timer事件回调函数
typedef void TimerCallback(EventLoop* loop, void* userData);


//封装一次IO触发的事件
struct IOEvent {
    IOEvent() {
        mask = 0;
        writeCallback = NULL;
        readCallback = NULL;
        readArgs_ = NULL;
        writeArgs_ = NULL;
    }

    //事件的读写属性
    int mask; //EPOLLIN, EPOLLOUT 

    //读事件触发所绑定的回调函数
    IOCallback *readCallback;  

    //写事件触发所绑定的回调函数
    IOCallback *writeCallback;
    
    //读事件回调函数的形参
    void *readArgs_;
    
    //写事件回调函数的形参
    void *writeArgs_;
};

struct TimerEvent//注册的Timer事件
{
    TimerEvent(TimerCallback* timercb, void* cbData, uint64_t argTS, uint32_t argInt = 0):
    tmCallback_(timercb), cbData_(cbData), ts_(argTS), interval_(argInt)
    {
    }
    TimerCallback* tmCallback_;
    void* cbData_;
    uint64_t ts_;
    uint32_t interval_;//interval millis
    int timerId_;
};
