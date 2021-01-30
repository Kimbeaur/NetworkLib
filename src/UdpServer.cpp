/**************************************************
 * @Description 
 * @Author kimbeaur
 * @Date 2021/1/30 09:42
 *************************************************/


#include "UdpServer.h"

//服务端处理客户端的读事件
void server_readCallback(EventLoop *loop, int fd, void *args)
{
    UdpServer *server = (UdpServer*)args;

    //处理读业务
    server->doRead();
}

UdpServer::UdpServer(EventLoop *loop, const char *ip, uint16_t port)
{
    //0. 忽略一些信号  SIGHUP, SIGPIPE
    if (signal(SIGHUP, SIG_IGN) == SIG_ERR)  {
        fprintf(stderr, "signal ignore SIGHUB\n");
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)  {
        fprintf(stderr, "signal ignore SIGHUB\n");
    }

    //创建套接字
    _sockfd = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC, IPPROTO_UDP);
    if (_sockfd == -1) {
        perror("create udp server");
        exit(1);
    }

    
    //2 初始化服务器的地址
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_aton(ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);


    //3 绑定端口
    if (bind(_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("udp server bind:");
        fprintf(stderr, "bind error\n");
        exit(1);
    }



    //4 添加loop事件监控
    this->_loop = loop;

    
    //5 清空客户端地址
    bzero(&_client_addr, sizeof(_client_addr));
    _client_addrlen = sizeof(_client_addr);


    //监控_sockfd 读事件，如果可读，代表客户端有数据过来
    _loop->AddIoEvent(_sockfd, server_readCallback, EPOLLIN, this);

    printf("udp server is running ip = %s, port = %d\n", ip, port);
}


//读客户端数据
void UdpServer::doRead() 
{
    while (true) {
        int pkg_len = recvfrom(_sockfd, _read_buf, sizeof(_read_buf), 0, (struct sockaddr*)&_client_addr, &_client_addrlen);
        if (pkg_len == -1) {
            if (errno == EINTR) {
                //中断错误
                continue; 
            }
            else if (errno == EAGAIN) {
                //非阻塞
                break;
            }
            else {
                perror("udp server recv from error");
                break;
            }
        }

        //数据已经读过来了
        MsgHead head; 
        //得到消息头部
        memcpy(&head, _read_buf, MESSAGE_HEAD_LEN);

        if (head.msglen > MESSAGE_LENGTH_LIMIT || head.msglen < 0 || head.msglen + MESSAGE_HEAD_LEN  != pkg_len) {
            //报文格式是有错误的
            fprintf(stderr, "error message head format error\n");
            continue;
        }

        //此时已经得到了一个完整的Message，通过msgid来调用对应的回调业务来处理
        _router.call(head.msgid, head.msglen, _read_buf + MESSAGE_HEAD_LEN, this);
    }
}

//udp 主动发送消息的方法
int UdpServer::SendMessage(const char *data, int msglen, int msgid)
{
    //udp server主动发包给客户端
    if (msglen > MESSAGE_LENGTH_LIMIT)  {
        fprintf(stderr, "too big message \n");
        return -1;
    }

    //制作一个message
    MsgHead head;
    head.msglen = msglen;
    head.msgid = msgid;

    //将head 写到输出缓冲中
    memcpy(_write_buf, &head, MESSAGE_HEAD_LEN);
    //将data 写到输出缓冲中
    memcpy(_write_buf + MESSAGE_HEAD_LEN, data, msglen);

    //通过_client_addr将报文发送给对方客户端
    int ret = sendto(_sockfd, _write_buf, msglen+ MESSAGE_HEAD_LEN, 0, (struct sockaddr*)&_client_addr, _client_addrlen);
    if (ret == -1) {
        perror("sendto () ..");
        return -1;
    }

    return ret;
}

//注册一个msgid和回调业务的路由关系 
void UdpServer::AddMsgRouter(int msgid, MsgCallback *cb, void *user_data)
{
    _router.RegisterMsgRouter(msgid, cb, user_data);
}

UdpServer::~UdpServer()
{
    _loop->DeleteIoEvent(_sockfd);
    close(_sockfd);
}
