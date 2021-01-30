/**************************************************
 * @Description 
 * @Author kimbeaur
 * @Date 2021/1/30 09:42
 *************************************************/

#pragma once

#include <ext/hash_map>
#include <ext/hash_set>
#include <sys/epoll.h>
#include <vector>
#include "EventBase.h"
#include "TimerQueue.h"

#define MAXEVENTS 10

//map:  key:fd ---> value:IOEvent
typedef __gnu_cxx::hash_map<int, IOEvent> IOEventMap;
typedef __gnu_cxx::hash_map<int, IOEvent>::iterator IOEventMapIter;

//set: value: fd 全部正在监听的fd集合
typedef __gnu_cxx::hash_set<int> listen_fd_set;


//task 一般任务的回掉函数类型
typedef void (*task_func)(EventLoop *loop, void *args);

class EventLoop
{
public:
    //构造 用于创建epoll
    EventLoop();

    //阻塞循环监听事件，并且处理
    void EventProcess();

    //添加一个io事件到EventLoop中
    void AddIoEvent(int fd, IOCallback *proc, int mask, void *args);
    //删除一个io事件 从EventLoop中
    void DeleteIoEvent(int fd);
    
    //删除一个io事件的某个触发条件(EPOLLIN/EPOLLOUT)
    void DeleteIoEvent(int fd, int mask);

    // -=====> 针对 异步task任务的方法
    //添加一个task任务到_ready_tasks集合中
    void AddTask(task_func func, void *args);

    //执行全部ready_tasks里面的任务
    void ExecuteReadyTasks();

    //获取当前EventLoop中正在监听的fd集合
    void GetListenFds(listen_fd_set &fds) {
        fds = listen_fds;
    }

    //定时器事件函数
    int RunAt(TimerCallback cb, void* args, uint64_t ts);
    int RunAfter(TimerCallback cb, void* args, int sec, int millis = 0);
    int RunEvery(TimerCallback cb, void* args, int sec, int millis = 0);
    void DelTimer(int timerId);


private:
    int _epfd;//通过epoll_create来创建的
    
    //当前EventLoop 监控的fd和对应事件的关系
    IOEventMap _io_evs; 

    //当前EventLoop 都有哪些fd正在监听 epoll_wait()正在等待哪些fd触发状态
    //作用是将当前的服务器可以主动发送消息给客户端
    //当前的fd集合中表示 在线的fd
    listen_fd_set listen_fds; 
    
    //每次epoll_wait返回触发，所返回的被激活的事件集合 
    struct epoll_event _fired_evs[MAXEVENTS]; 

    // -=====> 针对 异步task任务的属性
    typedef std::pair<task_func, void*> task_func_pair;

    //目前已经就绪的全部的task任务
    std::vector<task_func_pair> _ready_tasks;

    //定时器对列
    TimerQueue *timerQueue_;

    //定时器队列回调函数
    friend void TimerQueueCallback(EventLoop* loop, int fd, void *args);

};
