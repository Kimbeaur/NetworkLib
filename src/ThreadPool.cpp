#include "ThreadPool.h"


//提供一个获取一个ThreadQueue方法
ThreadQueue<TaskMsg> * ThreadPool::getThread()
{
    if (_index == _thread_cnt)     {
        _index = 0;
    }
    printf("====> thread_num = %d <==== \n", _index);
    return _queues[_index++];
}


//IO事件触发的回调函数
//typedef void IOCallback(EventLoop *loop, int fd, void *args);

/*
 * 一旦有task任务消息过来， 这个业务函数就会被loop监听到并执行,读出消息队列里的消息并进行处理
 * */
void dealTask(EventLoop *loop, int fd, void *args)
{
    //1 从ThreadQueue中去取数据
    ThreadQueue<TaskMsg>* queue = (ThreadQueue<TaskMsg>*)args;

    //取出queue 调用recv()方法
    std::queue<TaskMsg> new_task_queue;
    queue->recv(new_task_queue);//new_task_queue就是存放的 ThreadQueue的全部任务

    //依次处理每个任务
    while (new_task_queue.empty() != true) {
        //从队列中得到一个任务
        TaskMsg task = new_task_queue.front();

        //将这个任务和队列中弹出
        new_task_queue.pop();

        if (task.type == TaskMsg::NEW_CONN) {
            //如果是任务1: 中取出来的数据 就应该是一个刚刚accept成功的connfd
            TcpConn *conn = new TcpConn(task.connfd, loop);
            //让当前线程去创建一个链接，同时将这个链接的connfd加入到当前thrad的EventLoop中进行监听
            if (conn == NULL) {
                fprintf(stderr, "in thread new TcpConn error\n");
                exit(1);
            }

            printf("[thread]: create net connection succ !\n");
        }
        else if (task.type == TaskMsg::NEW_TASK) {
            //如果是任务2: 是一个普通任务
            //将该任务添加到EventLoop 循环中，去执行
            loop->AddTask(task.TaskCallback, task.args);
        }
        else {
            fprintf(stderr, "unknown task!\n");
        }
    }
}


//线程的主业务函数
void *ThreadMain(void *args)
{
    ThreadQueue<TaskMsg> *queue = (ThreadQueue<TaskMsg>*)args;

    EventLoop *loop = new EventLoop();
    if (loop == NULL) {
        fprintf(stderr, "new EventLoop error");
        exit(1);
    }

    queue->setLoopEvent(loop);
    queue->setCallback(dealTask, queue);

    //启动loop监听
    loop->EventProcess();

    return NULL;
}


//初始化线程池的构造函数 每次创建一个server 就应该创建一个线程池
ThreadPool::ThreadPool(int thread_cnt)
{
    _index = 0;
    _queues = NULL;
    _thread_cnt = thread_cnt;
    if (_thread_cnt <= 0) {
        fprintf(stderr, "thread_cnt need > 0\n");
        exit(1);
    }

    //创建ThreadQueue
    _queues = new ThreadQueue<TaskMsg>*[thread_cnt];// 开辟了 cnt个 ThreadQueue指针，每个指针并没有真正new对象
    _tids = new pthread_t[thread_cnt];


    //开辟线程
    int ret;
    for (int i = 0; i < thread_cnt; i++) {
        //给一个ThreadQueue开辟内存，创建对象
        _queues[i] = new ThreadQueue<TaskMsg>();

        //第i个线程 绑定第i个thraed_queue
        ret = pthread_create(&_tids[i], NULL, ThreadMain, _queues[i]);
        if (ret == -1){
            perror("ThreadPool create error");
            exit(1);
        }

        printf("create %d thread\n", i);

        //将线程设置detach模式 线程分离模式
        pthread_detach(_tids[i]);
    }

    printf("====> new ThreadPool <==== \n");
    
    return ;
}


//发送一个task NEW_TASK类型的任务接口
void ThreadPool::sendTask(task_func func, void *args)
{
    //给当前ThreadPool中每一个thread里的ThreadQueue去发送当前task
    TaskMsg task;

    //制作一个任务
    task.type = TaskMsg::NEW_TASK;
    task.TaskCallback = func;
    task.args = args;

    for (int i = 0; i < _thread_cnt; i++) {
        //取出第i个线程的消息队列queue
        ThreadQueue<TaskMsg> *queue = _queues[i];

        //给queue发送一个task任务
        queue->send(task);
    }
}

