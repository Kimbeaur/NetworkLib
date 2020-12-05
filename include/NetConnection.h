#pragma once

/*
 *
 * 抽象的链接类 
 *
 * */
class NetConnection 
{
public:
    NetConnection() {}

    //纯虚函数 
    virtual int SendMessage(const char *data, int msglen, int msgid) = 0;
    virtual int GetFd() = 0;

    void * param; //开发者可以通过该参数传递一些动态的自定义参数
};

//创建链接/销毁链接 要触发的回调函数的 函数类型
typedef  void (*ConnCallback)(NetConnection *conn, void *args);
