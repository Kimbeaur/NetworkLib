/**************************************************
 * @Description 
 * @Author kimbeaur
 * @Date 2021/1/30 09:42
 *************************************************/

#pragma once

#include <netinet/in.h>
#include "EventLoop.h"
#include "TcpConn.h"
#include "Message.h"
#include "ThreadPool.h"

class TcpServer
{
public:
    //构造函数
    TcpServer(EventLoop * loop, const char *ip, uint16_t port);

    //开始提供创建链接的服务
    void doAccept();

    //析构函数  资源的释放
    ~TcpServer();

private:
    int _sockfd; //套接字 listen fd
    struct sockaddr_in  _connaddr; //客户端链接地址
    socklen_t _addrlen; //客户端链接地址长度


    //EventLoop epoll的多路IO复用机制
    EventLoop *_loop;

public:
    static TcpConn*  *conns; //全部已经在线的连接 
    static void increaseConn(int connfd, TcpConn *conn); //新增一个链接
    static void decreaseConn(int connfd); //减少一个链接
    static void getConnNumber(int *curr_conn); //得到当前的连接刻度

    static pthread_mutex_t _conns_mutex;//保护conns添加删除的互斥锁
#define MAX_CONNS 3 //从配置文件中读的 TODO
    static int _max_conns; //当前允许连接的最大个数
    static int _curr_conns; //当前所管理的连接个数

    //添加一个路由分发机制句柄
    static MsgRouter router;

    //提供一个添加路由的方法 给 开发者提供的API
    void AddMsgRouter(int msgid, MsgCallback *cb, void *user_data = NULL) {
        router.RegisterMsgRouter(msgid, cb, user_data);
    }


    //设置链接创建之后Hook函数  API
    static void setConnStart(ConnCallback cb, void *args = NULL)
    {
        conn_start_cb = cb;
        conn_start_cb_args = args;
    }

    //设置链接销毁之前Hook函数 API
    static void setConnClose(ConnCallback cb, void *args = NULL)
    {
        conn_close_cb = cb;
        conn_close_cb_args = args;
    }

    //创建链接之后 要触发的回调函数
    static ConnCallback conn_start_cb;      
    static void * conn_start_cb_args;

    //销毁链接之前 要触发的回调函数
    static ConnCallback conn_close_cb;
    static void * conn_close_cb_args;

    ThreadPool * getThreadPool() {
        return _ThreadPool;
    }

private:
    //==========链接池=========
    ThreadPool *_ThreadPool;
};


