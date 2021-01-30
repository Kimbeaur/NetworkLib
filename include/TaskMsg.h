
/**************************************************
 * @Description 
 * @Author kimbeaur
 * @Date 2021/1/30 09:42
 *************************************************/
#pragma once


/*
 *  ThreadQueue消息队列 所能够接收的消息类型
 * */
struct TaskMsg
{
    //有两大类task任何类型
    // 一、 新建立链接的任务
    // 二、 一般的普通任务 //主thread希望分发一些任务给 每个线程来处理
    enum TASK_TYPE
    {
        NEW_CONN, //新建链接的任务
        NEW_TASK //一般的任务
    };

    TASK_TYPE type; //任务类型
     

    //任务本身数据内容
    union {
        //如果是任务1 ，TaskMsg的任务内容就是一个 刚刚创建好的connfd
        int connfd;

        //暂时用不上
        //如果是任务2, TaskMsg的任务内容应该由 具体的数据参数 和 具体的回调业务
        struct {
            void (*TaskCallback)(EventLoop* loop, void *args);
            void *args;
        };
    };
};
