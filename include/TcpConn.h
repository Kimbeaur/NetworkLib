/**************************************************
 * @Description 
 * @Author kimbeaur
 * @Date 2021/1/30 09:42
 *************************************************/

#pragma once

#include "EventLoop.h"
#include "ReactorBuffer.h"
#include "NetConnection.h"

class TcpConn :public NetConnection
{
public:
    //初始化conn
    TcpConn(int connfd, EventLoop *loop);
    
    // 被动处理读业务的方法 由EventLoop监听触发的
    void doRead();
    
    // 被动处理写业务的方法 有EventLoop监听触发的
    void doWrite();
    
    // 主动发送消息的方法
    virtual int SendMessage(const char *data, int msglen, int msgid);

    virtual int GetFd()
    {
        return this->_connfd;
    }
    
    // 销毁当前链接
    void cleanConn();
    
private:
    //当前的链接conn 的套接字fd
    int _connfd;
    
    //当前链接是归属于哪个EventLoop的
    EventLoop *_loop;
    
    //输出OutputBuf
    OutputBuf obuf; 
    
    //输入input_buf
    InputBuf ibuf;
};
