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

class UdpClient:public NetConnection
{
public:

    UdpClient(EventLoop *loop, const char *ip, uint16_t port);

    ~UdpClient();

    //udp 主动发送消息的方法
    virtual int SendMessage(const char *data, int msglen, int msgid);
    virtual int GetFd() {
        return this->_sockfd;
    }

    void doRead();

    void AddMsgRouter(int msgid, MsgCallback *cb, void *user_data = NULL);

private:
    int _sockfd;

    char _read_buf[MESSAGE_LENGTH_LIMIT];
    char _write_buf[MESSAGE_LENGTH_LIMIT];

    EventLoop *_loop;

    //消息路由分发机制
    MsgRouter _router;
};
