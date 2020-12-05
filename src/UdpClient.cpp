#include "UdpClient.h"


//客户端处理客户端的读事件
void client_readCallback(EventLoop *loop, int fd, void *args)
{
    UdpClient *client = (UdpClient*)args;

    //处理读业务
    client->doRead();
}

UdpClient::UdpClient(EventLoop *loop, const char *ip, uint16_t port)
{
    //创建套接字
    _sockfd = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC, IPPROTO_UDP);
    if (_sockfd == -1) {
        perror("create udp client");
        exit(1);
    }


    //2 初始化要链接服务器的地址
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_aton(ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    //2 链接服务器
    int ret = connect(_sockfd, (const struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        perror("connect ");
        exit(1);
    }

    //3 添加一个读事件回调
    this->_loop = loop;
    _loop->AddIoEvent(_sockfd, client_readCallback, EPOLLIN, this);
}

UdpClient::~UdpClient()
{
    _loop->DeleteIoEvent(_sockfd) ;
    close(_sockfd);
}

void UdpClient::AddMsgRouter(int msgid, MsgCallback *cb, void *user_data)
{
    _router.RegisterMsgRouter(msgid, cb, user_data);
}

//udp 主动发送消息的方法
int UdpClient::SendMessage(const char *data, int msglen, int msgid)
{
    //udp client主动发包给客户端
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
    int ret = sendto(_sockfd, _write_buf, msglen+ MESSAGE_HEAD_LEN, 0, NULL, 0);
    if (ret == -1) {
        perror("sendto () ..");
        return -1;
    }

    return ret;
}

void UdpClient::doRead()
{
    while (true) {
        int pkg_len = recvfrom(_sockfd, _read_buf, sizeof(_read_buf), 0, NULL, NULL);
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
                perror("udp client recv from error");
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

