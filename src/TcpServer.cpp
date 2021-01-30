/**************************************************
 * @Description 
 * @Author kimbeaur
 * @Date 2021/1/30 09:42
 *************************************************/

#include <iostream>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <strings.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "TcpServer.h"
#include "TcpConn.h"
#include "ReactorBuffer.h"
#include "ConfigFile.h"

// =========== 针对连接管理的初始化 ============
TcpConn* *TcpServer::conns = NULL;
pthread_mutex_t TcpServer::_conns_mutex = PTHREAD_MUTEX_INITIALIZER;
int TcpServer::_curr_conns = 0;
int TcpServer::_max_conns = 0;

//  ========= 初始化 路由分发机制句柄 ======
MsgRouter TcpServer::router; //调用的无参构造

// =========== 初始化 Hook函数 =====
ConnCallback TcpServer::conn_start_cb = NULL;
ConnCallback TcpServer::conn_close_cb = NULL;
void * TcpServer::conn_start_cb_args = NULL;
void * TcpServer::conn_close_cb_args = NULL;


void TcpServer::increaseConn(int connfd, TcpConn *conn) //新增一个链接 12 , 17 , 26
{
    pthread_mutex_lock(&_conns_mutex);
    conns[connfd] = conn;
    _curr_conns++;
    pthread_mutex_unlock(&_conns_mutex);
}
void TcpServer::decreaseConn(int connfd) //减少一个链接  
{
    pthread_mutex_lock(&_conns_mutex);
    conns[connfd]  = NULL;
    _curr_conns--;
    pthread_mutex_unlock(&_conns_mutex);
}
void TcpServer::getConnNumber(int *curr_conn) //得到当前的连接刻度
{
    pthread_mutex_lock(&_conns_mutex);
    *curr_conn = _curr_conns;
    pthread_mutex_unlock(&_conns_mutex);
}

// ============ 以上是连接管理相关函数　========

void lars_hello()
{
    std::cout <<"lars Hello" <<std::endl;
}


#if 0
//临时的收发消息结构
struct message
{
    char data[m4K];
    char len;
};
struct message msg;
#endif

//typedef void IOCallback(EventLoop *loop, int fd, void *args);
void accept_callback(EventLoop *loop, int fd, void *args)
{
    TcpServer *server = (TcpServer*)args;
    server->doAccept();
}




//构造函数
TcpServer::TcpServer(EventLoop *loop, const char *ip, uint16_t port)
{
    _ThreadPool = NULL;

    //0. 忽略一些信号  SIGHUP, SIGPIPE
    if (signal(SIGHUP, SIG_IGN) == SIG_ERR)  {
        fprintf(stderr, "signal ignore SIGHUB\n");
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)  {
        fprintf(stderr, "signal ignore SIGHUB\n");
    }
    
    //1. 创建socket
    _sockfd = socket(AF_INET, SOCK_STREAM |SOCK_CLOEXEC, IPPROTO_TCP) ;
    if (_sockfd == -1) {
        fprintf(stderr, "tcp::server :socket()\n");
        exit(1);
    }



    //2 初始化服务器的地址
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_aton(ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);


    //2.5 设置sock可以重复监听
    int op = 1;
    if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) < 0) {
        fprintf(stderr, "set socketopt reuse error\n");
    }
    
    //3 绑定端口
    if (bind(_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "bind error\n");
        exit(1);
    }
    
    //4 监听
    if (listen(_sockfd, 500) == -1) {
        fprintf(stderr , "listen error\n");
        exit(1);
    }

    //5 将形参loop添加到 TcpServer _loop中
    _loop = loop;

    //6 创建线程池
    //从配置文件中读取 (建议配置的个数和用来网络通信cpu数量一致)
    int thread_cnt = ConfigFile::instance()->GetNumber("reactor", "threadNum", 3);
    if (thread_cnt > 0) {
        _ThreadPool = new ThreadPool(thread_cnt);
        if (_ThreadPool == NULL) {
            fprintf(stderr, "tcp server new ThreadPool error\n");
            exit(1);
        }
    }

    //7 创建连接管理
    _max_conns = ConfigFile::instance()->GetNumber("reactor", "maxConn", 1000);
    conns = new TcpConn* [_max_conns+5+2*thread_cnt]; //3表示 stdin, stdout, stderr, 3个进程默认的文件描述符,因为这里面conns每个索引值就是当前的connfd标号
    //stdin, stdout, stderr
    //listenfd,
    //main-thread:epoll_fd
    //sub_thread:epoll_fd *thread_cnt
    //ThreadQueue:evfd * thread_cnt
    
    
    if (conns == NULL) {
        fprintf(stderr, "new conns[%d] error\n", _max_conns);
        exit(1);
    }


    //8 注册_sockfd 读事件---> accept处理
    _loop->AddIoEvent(_sockfd, accept_callback, EPOLLIN, this);
}

//开始提供创建链接的服务
void TcpServer::doAccept()
{
    int connfd;
    while (true) {
        //1 accept
        _addrlen = 0;

        connfd = accept(_sockfd, (struct sockaddr*)&_connaddr, &_addrlen);
        if (connfd == -1) {
            if (errno == EINTR)  {
                //中断错误
                fprintf(stderr, "accept errno = EINTR\n");
                continue;
            }
            else if (errno == EAGAIN) {
                fprintf(stderr, "accept errno = EAGAIN\n");
                break;
            }
            else if (errno == EMFILE) {
                //建立链接过多， 资源不够
                fprintf(stderr, "accept errno = EMFILE\n");
                continue;
            }
            else {
                fprintf(stderr, "accept error\n");
                perror("accept:\n");
                exit(1);
            }
        }
        else {
            //accept succ!
            //判断连接个数是否已经超过最大值 _max_conns;
            int cur_conns;
            getConnNumber(&cur_conns); //获取当前在线的连接个数
            if (cur_conns >= _max_conns) {
                fprintf(stderr, "so many connections, max = %d\n", _max_conns);
                close(connfd);
            }
            else {
                if (_ThreadPool != NULL) {
                    //开启了多线程模式
                    //将这个connfd交给一个线程来去创建并且去监听
                    //1 从线程池中获取一个ThreadQueue
                    ThreadQueue<TaskMsg> *queue = _ThreadPool->getThread();
                    //创建一个任务
                    TaskMsg task;
                    task.type = TaskMsg::NEW_CONN;
                    task.connfd = connfd;

                    //2 将connfd 发送到ThreadQueue中
                    queue->send(task);
         
                }
                else {
                    //没有开启多线程模式
                    //创建一个新的TcpConn连接对象 将当前的链接用TcpServer来监听，是一个单线程的server
                    TcpConn *conn = new TcpConn(connfd, _loop);
                    if (conn == NULL) {
                        fprintf(stderr, "new TcpConn error\n");
                        exit(1);
                    }
                    break; 
                }
            }
        }
    }
}

//析构函数  资源的释放
TcpServer::~TcpServer()
{
    close(_sockfd);
}
