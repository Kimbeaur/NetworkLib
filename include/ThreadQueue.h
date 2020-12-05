#pragma once
#include <queue>
#include <pthread.h>
#include <stdio.h>
#include "EventLoop.h"
#include <sys/eventfd.h>
#include <unistd.h>


template <typename T>
class ThreadQueue
{
public:
    ThreadQueue()
    {
        _loop = NULL;
        pthread_mutex_init(&_queue_mutex, NULL);
        //创建一个fd 用来被监听的 能够被epoll监听的fd，没有跟磁盘相关联，也没有跟socket相关联
        _evfd = eventfd(0, EFD_NONBLOCK);
        if (_evfd == -1) {
            perror("eventfd()");
            exit(1);
        }
    }

    ~ThreadQueue() {
        pthread_mutex_destroy(&_queue_mutex);
        close(_evfd);
    }


    //向队列中添加一个任务 (main_thread调用获取其他业务功能线程来调用)
    void send(const T & task) {
        //将task加入到queue中 ，激活_evfd
        pthread_mutex_lock(&_queue_mutex);
        _queue.push(task);

        //激活_evfd可读事件，向_evfd写数据
        unsigned long long idle_num = 1;
        int ret = write(_evfd, &idle_num, sizeof(unsigned long long));
        if (ret == -1) {
            perror("evfd write error");
        }
        pthread_mutex_unlock(&_queue_mutex);
    }

    //从队列中去数据 将整个queue返回给上层 queue_msgs传出型参数, 被_evfd激活的读事件业务函数来调用
    void recv(std::queue<T>& queue_msgs) {
        unsigned long long idle_num;

        //拿出queue数据 
        pthread_mutex_lock(&_queue_mutex);

        //evfd所绑定事件读写缓存清空，将占位激活符号，读出来
        int ret = read(_evfd, &idle_num, sizeof(unsigned long long));
        if (ret == -1) {
            perror("_evfd read");
        }

        //交换两个容器的指针 , 保证queue_msgs 是一个空队列,这样交换完之后， _queue才是一个空队列
        std::swap(queue_msgs, _queue);
        //std::copy(queue_msgs, _queue); //用拷贝也可以，性能没有swap好
        pthread_mutex_unlock(&_queue_mutex);
    }

    //让当前的ThreadQueue被哪个EventLoop所监听
    void setLoopEvent(EventLoop *loop) {
        this->_loop = loop;
    }


    //给当前对应设置一个_evfd如果出发EPOLLIN事件所 被激活的回调函数
    void setCallback(IOCallback *cb, void *args = NULL)
    {
        if (_loop != NULL) {
            _loop->AddIoEvent(_evfd, cb, EPOLLIN, args);
        }
    }

    
private:
    int _evfd; //让某个线程进行监听的 
    EventLoop *_loop;//目前是被哪个loop所监听
    std::queue<T> _queue; //队列
    pthread_mutex_t _queue_mutex; //保护queue的互斥锁
};
