#include "TcpClient.h"
#include <string.h>

//注册一个客户端处理服务器返回数据的回调业务
void callback_busi(const char *data, uint32_t len, int msgid, NetConnection* conn, void *user_data)
{
    //将数据写回去
    conn->SendMessage(data, len, msgid);
    printf("business send to server data[%s] len[%d] msgId[%d]",data,len, msgid);
}


//打印业务
void print_busi(const char *data, uint32_t len, int msgid, NetConnection *conn, void *user_data)
{
    printf("print busi is called!\n");
    printf("recv from server : [%s]\n", data);
    printf("msgid = %d\n", msgid);
    printf("len = %d\n", len);
}

//客户端创建连接之后hook业务
void on_client_build(NetConnection *conn, void *args)
{
    printf("==> on_client build ,send hello reactor to server!\n");
    //客户端一旦连接成功 就会主动server发送一个msgid = 1 的消息
    int msgid = 1;
    const char *msg = "hello reactor";

    conn->SendMessage(msg, strlen(msg), msgid);
}

//客户端销毁连接之前的hook业务
void on_client_lost(NetConnection *conn, void *args)
{
    printf("==> on_client lost !\n");
}

int main() 
{
    EventLoop loop;

    TcpClient *client = new TcpClient(&loop, "127.0.0.1", 8888);

    client->AddMsgRouter(1, print_busi);
    client->AddMsgRouter(2, callback_busi);
    client->AddMsgRouter(200, print_busi);
    client->AddMsgRouter(404, print_busi);


    //注册hook函数
    client->setConnStart(on_client_build);
    client->setConnClose(on_client_lost);

    loop.EventProcess();
}
