/**************************************************
 * @Description 
 * @Author kimbeaur
 * @Date 2021/1/30 09:42
 *************************************************/

#pragma once

#include <netinet/in.h>
#include <signal.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "NetConnection.h"
#include "EventLoop.h"
#include "Message.h"


class UdpServer : public NetConnection
{
public:

    UdpServer(EventLoop *loop, const char *ip, uint16_t port);

    //udp 主动发送消息的方法
    virtual int SendMessage(const char *data, int msglen, int msgid);
    virtual int GetFd()
    {
        return this->_sockfd;
    }

    //注册一个msgid和回调业务的路由关系 
    void AddMsgRouter(int msgid, MsgCallback *cb, void *user_data = NULL);

    ~UdpServer();

    //处理客户端数据的业务
    void doRead();

private:
    int _sockfd; 

    EventLoop *_loop;

    char _read_buf[MESSAGE_LENGTH_LIMIT];
    char _write_buf[MESSAGE_LENGTH_LIMIT];

    //客户单的ip地址
    struct sockaddr_in _client_addr;
    socklen_t _client_addrlen;

    //消息路由分发机制
    MsgRouter _router;
};

