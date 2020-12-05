#include "TcpClient.h"
#include "Message.h"
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>



void readCallback(EventLoop *loop, int fd, void *args)
{
    TcpClient *cli = (TcpClient*)args;
    cli->doRead();
}

void writeCallback(EventLoop *loop, int fd, void *args)
{
    TcpClient *cli = (TcpClient*)args;
    cli->doWrite();
}

TcpClient::TcpClient(EventLoop *loop, const char *ip, unsigned short port): _router() 
{
    _sockfd = -1; 
    _loop = loop;
    _conn_start_cb = NULL;
    _conn_close_cb = NULL;
    _conn_start_cb_args = NULL;
    _conn_close_cb_args = NULL;

    //封装即将要链接的远程server端的ip地址
    bzero(&_server_addr, sizeof(_server_addr));
    _server_addr.sin_family = AF_INET;
    inet_aton(ip, &_server_addr.sin_addr);
    _server_addr.sin_port = htons(port);
    _addrlen = sizeof(_server_addr);


    //链接远程客户端
    this->do_connect();
}

//发送方法
int TcpClient::SendMessage(const char *data, int msglen, int msgid)
{
    //printf("TcpClient::SendMessage()...\n");
    
    bool active_epollout = false;

    if (obuf.length() == 0) {
        active_epollout = true;
    }

    MsgHead head;
    head.msgid = msgid;
    head.msglen = msglen;

    //1 将消息头 写到Obuf中
    int ret = obuf.send_data((const char*)&head, MESSAGE_HEAD_LEN);
    if (ret != 0) {
        fprintf(stderr, "send head error\n");
        return -1;
    }

    // 2 写消息体
    ret = obuf.send_data(data, msglen);
    if (ret != 0) {
        fprintf(stderr, "send data error\n");
        //如果消息体写失败，消息头需要弹出重置
        obuf.pop(MESSAGE_HEAD_LEN);
        return -1;
    }

    if (active_epollout == true) {
        _loop->AddIoEvent(_sockfd, writeCallback, EPOLLOUT, this);
    }

    return 0;
}

//处理读业务
void TcpClient::doRead()
{
    //printf("TcpClient:: doRead()...\n");

    //1 从_sockfd中去读数据     
    int ret = ibuf.readData(_sockfd);
    if (ret == -1) {
        fprintf(stderr, "read data from socket\n");
        this->cleanConn();
        return;
    }
    else if (ret == 0) {
        //对端客户端正常关闭
        printf("peer server closed!\n");
        this->cleanConn();
    }

    //读出来的消息头
    MsgHead head;

    //2 读够来的数据，是否满足8字节 while
    while (ibuf.length() >= MESSAGE_HEAD_LEN) {
        // 2.1 先读头部 得到msgid,  msglen
        memcpy(&head, ibuf.data(), MESSAGE_HEAD_LEN);
        if (head.msglen > MESSAGE_LENGTH_LIMIT || head.msglen < 0) {
            fprintf(stderr, "data format error，too large or too small， need close\n");
            this->cleanConn();
            break;
        }

        // 2.2 判断得到的消息体的长度和头部里的长度是否一致
        if (ibuf.length() < MESSAGE_HEAD_LEN + head.msglen) {
            //缓存中buf剩余的收数据， 小于应该接受到的数据
            //说明当前不是一个完整的包
            break;
        }

        //表示当前包是合法的
        ibuf.pop(MESSAGE_HEAD_LEN);

#if 0
        if (_MsgCallback != NULL)  {
            this->_MsgCallback(ibuf.data(), head.msglen, head.msgid, this, NULL);
        }
#endif
        //调用注册的回调业务
        this->_router.call(head.msgid, head.msglen, ibuf.data(), this);
        
        //整个消息都处理完了
        ibuf.pop(head.msglen);
    }
    
    ibuf.adjust(); 

    return ;

}

//处理写业务
void TcpClient::doWrite()
{
    //printf("TcpClient:: doWrite()...\n");

    while (obuf.length()) {
        int write_num = obuf.write2fd(_sockfd);
        if (write_num == -1) {
            fprintf(stderr, "TcpClient write fd error\n");
            this->cleanConn();
            return;
        }
        else if (write_num == 0) {
            //当前不可写
            break;
        }
    }

    if (obuf.length() == 0) {
        //数据已经全部写完, 将_connfd的写事件删掉
        _loop->DeleteIoEvent(_sockfd, EPOLLOUT);
    }

    return ;
}

//释放链接
void TcpClient::cleanConn()
{
    //调用链接销毁之前的Hook函数
    if (_conn_close_cb != NULL){
        _conn_close_cb(this, _conn_close_cb_args);
    }

    if (_sockfd != -1) {
        _loop->DeleteIoEvent(_sockfd);
        close(_sockfd);
    }

    //重新发起链接
    //this->do_connect();
}

//如果该函数被执行，则链接创建成功
void connection_succ(EventLoop *loop, int fd, void *args)
{
    TcpClient *cli = (TcpClient*)args;
    loop->DeleteIoEvent(fd);

    //再对当前fd进行一次 错误编码的获取，如果没有任何错误，那么一定就是成功了
    //如果有的话， 认为fd是没有创建成功，链接创建失败
    int result = 0;
    socklen_t result_len = sizeof(result);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &result, &result_len);
    if (result == 0) {
        //fd中没有任何错误
        //链接创建成功
        //printf("connection succ!!\n") ;

        //链接创建成功之后的一些业务
        if (cli->_conn_start_cb != NULL) {
            cli->_conn_start_cb(cli, cli->_conn_start_cb_args);
        }

        //添加针对当前cli fd的 读回调
        loop->AddIoEvent(fd, readCallback, EPOLLIN, cli);
         
        if (cli->obuf.length() != 0) {
            //让EventLoop触发写回调业务
            loop->AddIoEvent(fd, writeCallback, EPOLLOUT, cli);
        }
    }
    else {
        //链接创建失败
        fprintf(stderr, "connection %s:%d error\n", inet_ntoa(cli->_server_addr.sin_addr), ntohs(cli->_server_addr.sin_port));
        return ;
    }
}


//链接服务器
void TcpClient::do_connect()
{
    //connect...
    if (_sockfd != -1)  {
        close(_sockfd);
    }

    //创建套接字(非阻塞)
    _sockfd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK, IPPROTO_TCP);
    if (_sockfd == -1) {
        fprintf(stderr, "create tcp client socket error\n");
        exit(1);
    }

    int ret = connect(_sockfd, (const struct sockaddr *)&_server_addr, _addrlen);
    if (ret == 0) {
        //创建链接成功
        printf("connect ret == 0, connect %s:%d succ!\n", inet_ntoa(_server_addr.sin_addr), ntohs(_server_addr.sin_port));

        connection_succ(_loop, _sockfd, this);
    }
    else {
        //创建链接失败
        if (errno == EINPROGRESS) {
            //fd是非阻塞的， 会出现这个错误，单并不是表示创建链接失败
            //如果fd是可写的 ，那么链接创建是成功的
            //printf("do_connect, EINPROGRESS\n");
            
            //让EventLoop去检测当前的sockfd是否可写，如果当前回调被执行，说明可写，那么链接则创建成功
            _loop->AddIoEvent(_sockfd, connection_succ, EPOLLOUT, this);
        }
        else {
            fprintf(stderr, "connection error\n");
            exit(1);
        }
    }
}

