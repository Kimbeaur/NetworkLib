/**************************************************
 * @Description 
 * @Author kimbeaur
 * @Date 2021/1/30 09:42
 *************************************************/

#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ReactorBuffer.h"
#include "EventLoop.h"
#include "Message.h"
#include "NetConnection.h"


class TcpClient :public NetConnection
{
public:
    TcpClient(EventLoop *loop, const char *ip, unsigned short port);

    //发送方法
    virtual int SendMessage(const char *data, int msglen, int msgid);

    virtual int GetFd() {
        return this->_sockfd;
    }

    //处理读业务
    void doRead();

    //处理写业务
    void doWrite();

    //释放链接
    void cleanConn();

    //链接服务器
    void do_connect();

    //注册消息路由回调函数
    void AddMsgRouter(int msgid, MsgCallback *cb, void *user_data = NULL) {
        _router.RegisterMsgRouter(msgid, cb, user_data);
    }

    //输入缓冲
    InputBuf ibuf;
    //输出缓冲
    OutputBuf obuf;
    //server端ip地址
    struct sockaddr_in _server_addr;
    socklen_t _addrlen;


    void setConnStart(ConnCallback cb, void *args = NULL) {
        _conn_start_cb = cb;
        _conn_start_cb_args = args;
    }

    void setConnClose(ConnCallback cb, void *args = NULL) {
        _conn_close_cb = cb;
        _conn_close_cb_args = args;
    }

    //创建链接之后的触发的 回调函数
    ConnCallback _conn_start_cb;
    void * _conn_start_cb_args;

    //销毁链接之前触发的 回调函数
    ConnCallback _conn_close_cb;
    void *_conn_close_cb_args;

private:
    int _sockfd; //当前客户端链接

    //客户端事件的处理机制
    EventLoop *_loop;

    //消息分发机制
    MsgRouter _router;
};
