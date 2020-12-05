#pragma once


/*
 * 定义IO复用机制事件的封装(epoll原生事件来封装)
 *
 * */

class EventLoop;

//IO事件触发的回调函数
typedef void IOCallback(EventLoop *loop, int fd, void *args);


/*
 * 封装一次IO触发的事件
 * */
struct IOEvent {
    IOEvent() {
        mask = 0;
        write_callback = NULL;
        read_callback = NULL;
        rcb_args = NULL;
        wcb_args = NULL;
    }

    //事件的读写属性
    int mask; //EPOLLIN, EPOLLOUT 

    //读事件触发所绑定的回调函数
    IOCallback *read_callback;  

    //写事件触发所绑定的回调函数
    IOCallback *write_callback;
    
    //读事件回调函数的形参
    void *rcb_args;
    
    //写事件回调函数的形参
    void *wcb_args;
};
