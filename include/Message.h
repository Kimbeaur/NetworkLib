#pragma once

#include <iostream>
#include <ext/hash_map>
#include "TcpConn.h"

//解决tcp粘包问题 的消息封装头
struct MsgHead 
{
    int msgid;  //当前的消息类型
    int msglen; //消息体的长度
};


//消息头的长度，固定值
#define MESSAGE_HEAD_LEN 8

//消息头+消息体 最大长度限制
#define MESSAGE_LENGTH_LIMIT (65535 - MESSAGE_HEAD_LEN)

//定义一个路由回调函数的数据类型
typedef void MsgCallback(const char *data, uint32_t len, int msgid, NetConnection *conn, void *user_data);



/*
 * 定义一个消息路由分发机制
 *
 * */
class MsgRouter
{
public:

    //MsgRoout构造函数， 初始化两个map
    MsgRouter(): _router(),_args() {
        printf("MsgRoout init...\n");
    }

    //给一个消息ID注册一个对应的回调业务函数
    int RegisterMsgRouter(int msgid, MsgCallback *msg_cb, void *user_data) {
        if (_router.find(msgid) != _router.end())  {
            //该msgid 的回调业务已经存在
            std::cout << "msgID " << msgid << " is already register..." << std::endl;
            return -1;
        }

        //std::cout <<"Add msg callback msgid = " << msgid << std::endl;

        _router[msgid] = msg_cb;
        _args[msgid] = user_data;

        return 0;
    }

    //调用注册的回调函数业务
    void call(int msgid, uint32_t msglen, const char *data, NetConnection *conn) {
        //std::cout << "call msgid = " << msgid << std::endl;

        if (_router.find(msgid) == _router.end()) {
            std::cout << "msgid " << msgid << " is not register" << std::endl;
            return;
        }

        //直接取出回调函数 ，并且执行
        MsgCallback* callback = _router[msgid];
        void *user_data = _args[msgid];
        //调用注册的回调业务
        callback(data, msglen, msgid, conn, user_data);
    }

private:
    //针对消息的路由分发， key是msgid， value是注册的回调业务处理函数
    __gnu_cxx::hash_map<int, MsgCallback*> _router;

    //每个路由业务函数对应的形参
    __gnu_cxx::hash_map<int, void*> _args;
};
