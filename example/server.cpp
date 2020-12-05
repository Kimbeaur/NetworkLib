#include "TcpServer.h" 
#include "string.h"
#include "ConfigFile.h"

/*
struct player
{
    NetConnection *conn;//可以跟对应的客户端的玩家进行通信
};
*/

TcpServer *server;

//当前任务的一个task callback
void print_task(EventLoop* loop, void *args)
{
    printf("======== Active Task Func ===== \n");

    //得到当前thread的在线的fd有哪些
    listen_fd_set fds; 
    loop->GetListenFds(fds);//从当前线程的loop获取，每个线程所监听的fd是不同的

    listen_fd_set::iterator it;  
    for (it = fds.begin(); it != fds.end(); it++) {
        int fd = *it; //当前线程已经建立连接并且监听中的在线的客户端
        
        TcpConn *conn = TcpServer::conns[fd];//取出连接
        if (conn != NULL)  {
            int msgid = 404;        
            const char *msg = "Hello I am a Task!";
            conn->SendMessage(msg, strlen(msg), msgid);
        }
    }
}

//定义一个回显的业务
void callback_busi(const char *data, uint32_t len, int msgid, NetConnection *conn, void *user_data)
{
    //直接将数据发回去
    //此时NetConnection*conn--->  TcpConn对象 ,conn就是能够跟对端客户端通信的TcpConn链接对象
    conn->SendMessage(data, len, msgid);

    printf("server conn param = %s\n", (char*)conn->param);
}

//打印业务
void print_busi(const char *data, uint32_t len, int msgid, NetConnection *conn, void *user_data)
{
    printf("recv from client : [%s]\n", data);
    printf("msgid = %d\n", msgid);
    printf("len = %d\n", len);

  
}

//新客户端创建成功之后的回调
void on_client_build(NetConnection *conn, void *args)
{
    printf("===> on_client_build is called!\n");
    int msgid = 200;
    const char *msg = "welcome ! you are online msgid200 ";

    conn->SendMessage(msg, strlen(msg), msgid);

    int msgid1 = 2;
    const char *msg1 = "welcome ! you are online msgid2 ";

     conn->SendMessage(msg1, strlen(msg1), msgid1);

    //每次客户端在创建连接成功之后，执行一个任务
    server->getThreadPool()->sendTask(print_task);    


    //给当前的conn绑定一个自定义的参数，供之后来使用
    const char *conn_param_test = "I am the conn param for you!";
    conn->param = (void*)conn_param_test;
}

//客户端断开之前的回调
void on_client_lost(NetConnection *conn, void *args)
{
    printf("===> on_client_lost is called!\n");
    printf("connection is lost !\n");
}



int main()
{
    EventLoop loop;

    //--->加载配置文件
    ConfigFile::setPath("./reactor.ini");
    std::string ip = ConfigFile::instance()->GetString("reactor", "ip", "0.0.0.0");
    short port = ConfigFile::instance()->GetNumber("reactor", "port", 8888);


    server = new TcpServer(&loop, ip.c_str(), port);
    
    //注册一些回调方法
    server->AddMsgRouter(1, callback_busi);
    server->AddMsgRouter(2, print_busi);

    //注册链接hook函数
    server->setConnStart(on_client_build);
    server->setConnClose(on_client_lost);

    loop.EventProcess();

    return 0;
}
