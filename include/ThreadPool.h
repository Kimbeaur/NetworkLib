#pragma  once

#include "ThreadQueue.h"
#include "TaskMsg.h"
#include "TcpConn.h"

class ThreadPool
{
public:

    //初始化线程池的构造函数
    ThreadPool(int thread_cnt);

    //提供一个获取一个ThreadQueue方法
    ThreadQueue<TaskMsg> * get_thread();


    //发送一个task NEW_TASK类型的任务接口
    void sendTask(task_func func, void *args = NULL);


private:
    //当前的ThreadQueue集合
    ThreadQueue<TaskMsg>** _queues;

    //线程个数
    int _thread_cnt;

    //线程ID
    pthread_t*  _tids;

    //获取线程的index索引
    int _index;
};
