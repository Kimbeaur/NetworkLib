
#include <unistd.h>
#include "EventLoop.h"


void TimerQueueCallback(EventLoop *loop, int fd, void *args)
{
    std::vector<TimerEvent> fired_evs;

    loop->timerQueue_->GetTimerOut(fired_evs);
    for (std::vector<TimerEvent>::iterator it = fired_evs.begin();
        it != fired_evs.end(); ++it)
    {
        it->tmCallback_(loop, it->cbData_);
    }
}
//构造 用于创建epoll
EventLoop::EventLoop()
{
    //创建一个epoll句柄 文件描述符
    _epfd = epoll_create1(0);
    if (_epfd == -1) {
        fprintf(stderr, "epoll create error\n");
        exit(1);
    }
    //初始化定时器队列
    timerQueue_ = new TimerQueue();
     if (timerQueue_ == nullptr) {
        fprintf(stderr, "when new TimerQueue\n");
        exit(1);
    }
    //注册定时器事件加入到EventLoop
    AddIoEvent(timerQueue_->GetTimerfd(), TimerQueueCallback, EPOLLIN, timerQueue_);
    //初始化清零
    _ready_tasks.clear();
}

//阻塞循环监听事件，并且处理 epoll_wait, 包含调用对应的触发回调函数
void EventLoop::EventProcess()
{
    IOEventMapIter ev_it;
    while (true) {

        int nfds = epoll_wait(_epfd, _fired_evs, MAXEVENTS, -1);

       	if (-1 == nfds)
		{
			if (EINTR == errno)
			{
				continue;
			}
			else
			{
				break;
			}
		}

        for (int i = 0; i < nfds; i ++) {
            //通过epoll触发的fd 从map中找到对应的IOEvent
            ev_it = _io_evs.find(_fired_evs[i].data.fd);

            //取出对应的事件
            IOEvent *ev = &(ev_it->second);

            if (_fired_evs[i].events & EPOLLIN) {
                //读事件， 调用读回调函数
                void *args = ev->readArgs_;
                //调用读业务
                ev->readCallback(this, _fired_evs[i].data.fd, args);
            }
            else if (_fired_evs[i].events & EPOLLOUT) {
                //写事件, 调用写回调函数
                void *args = ev->writeArgs_;
                ev->writeCallback(this, _fired_evs[i].data.fd, args);
            }
            else if (_fired_evs[i].events & (EPOLLHUP|EPOLLERR)) {
                //水平触发未处理， 可能会出现HUP事件，需要正常处理读写， 如果当前时间events 既没有写，也没有读 将events从epoll中删除
                if (ev->readCallback != NULL) {
                    //读事件， 调用读回调函数
                    void *args = ev->readArgs_;
                    //调用读业务
                    ev->readCallback(this, _fired_evs[i].data.fd, args);
                }
                else if (ev->writeCallback != NULL) {
                    //写事件, 调用写回调函数
                    void *args = ev->writeArgs_;
                    ev->writeCallback(this, _fired_evs[i].data.fd, args);
                }
                else  {
                    //删除
                    fprintf(stderr, "fd %d get error， delete from epoll", _fired_evs[i].data.fd);
                    this->DeleteIoEvent(_fired_evs[i].data.fd);
                }
            }
        }

        //每次全部的fd的读写事件执行完
        //在此处，执行一些 其他的task任务流程
        this->ExecuteReadyTasks();
    }
}

//添加一个io事件到EventLoop中 //添加和修改
void EventLoop::AddIoEvent(int fd, IOCallback *proc, int mask, void *args)
{ 
    int final_mask;
    int op;

    //1 找到当前的fd是否已经是已有事件
    IOEventMapIter it = _io_evs.find(fd);
    if (it == _io_evs.end()) {
        // 如果没有事件， ADD方式添加到epoll中
        op = EPOLL_CTL_ADD;    
        final_mask = mask;
    }
    else {
        // 如果事件存在, MOD方式添加到epoll中
        op = EPOLL_CTL_MOD;
        final_mask = it->second.mask | mask;
    }
    
    //2 将fd 和 IOCallback绑定 map中
    if (mask & EPOLLIN) {
        //该事件是一个读事件
        _io_evs[fd].readCallback = proc; //注册读回调业务
        _io_evs[fd].readArgs_ = args;
    }
    else if (mask & EPOLLOUT) {
        //该事件是一个写事件
        _io_evs[fd].writeCallback = proc; //注册写回调业务
        _io_evs[fd].writeArgs_ = args;
    }

    //****
    _io_evs[fd].mask = final_mask;
    
    //3 将当前的事件 加入到 原生的epoll
    struct epoll_event event;
    event.events = final_mask;
    event.data.fd = fd;
    if (epoll_ctl(_epfd, op, fd, &event) == -1) {
        fprintf(stderr, "epoll ctl %d error\n", fd);
        return;
    }
    
    //4 将当前fd加入到  正在监听的fd 集合中
    listen_fds.insert(fd);
}

//删除一个io事件 从EventLoop中
void EventLoop::DeleteIoEvent(int fd)
{
    IOEventMapIter it = _io_evs.find(fd);
    if (it == _io_evs.end()) {
        return;
    }

    //将事件从_io_evs map中删除  
    _io_evs.erase(fd);

    //将fd 从 listen_fds set 中删除
    listen_fds.erase(fd);

    //将fd 从 原生的epoll堆中删除
    epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL);
}

//删除一个io事件的某个触发条件(EPOLLIN/EPOLLOUT)
void EventLoop::DeleteIoEvent(int fd, int mask)
{
    IOEventMapIter it = _io_evs.find(fd);
    if (it == _io_evs.end()) {
        return;
    }

    //此时it就是当前要删除的事件对 key:fd value:IOEvent
    int &o_mask = it->second.mask;
    o_mask = o_mask & (~mask);

    if (o_mask == 0) {
        //通过删除条件已经没有触发条件
        this->DeleteIoEvent(fd);
    }
    else {
        //如果事件还存在，则修改当前事件
        struct epoll_event event;
        event.events = o_mask;
        event.data.fd = fd;

        epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &event);
    }
}



//添加一个task任务到_ready_tasks集合中
void EventLoop::AddTask(task_func func, void *args)
{
    task_func_pair func_pair(func, args);

    _ready_tasks.push_back(func_pair);
}

//执行全部ready_tasks里面的任务
void EventLoop::ExecuteReadyTasks()
{
    //遍历_ready_tasks 取出每个任务去执行
    std::vector<task_func_pair>::iterator it;

    for (it = _ready_tasks.begin(); it != _ready_tasks.end(); it++) {
        task_func func = it->first; //取出任务的执行函数
        void *args = it->second; //取出执行函数的参数

        //调用任务函数
        func(this, args);
    }


    //全部函数执行完毕， 清空当前的_ready_tasks 集合
    _ready_tasks.clear(); 
}

int EventLoop::RunAt(TimerCallback cb, void* args, uint64_t ts)
{
    TimerEvent te(cb, args, ts);
    return timerQueue_->AddTimer(te);
}

int EventLoop::RunAfter(TimerCallback cb, void* args, int sec, int millis)
{
    struct timespec tpc;
    clock_gettime(CLOCK_REALTIME, &tpc);
    uint64_t ts = tpc.tv_sec * 1000 + tpc.tv_nsec / 1000000UL;
    ts += sec * 1000 + millis;
    TimerEvent te(cb, args, ts);
    return timerQueue_->AddTimer(te);
}

int EventLoop::RunEvery(TimerCallback cb, void* args, int sec, int millis)
{
    uint32_t interval = sec * 1000 + millis;
    struct timespec tpc;
    clock_gettime(CLOCK_REALTIME, &tpc);
    uint64_t ts = tpc.tv_sec * 1000 + tpc.tv_nsec / 1000000UL + interval;
    TimerEvent te(cb, args, ts, interval);
    return timerQueue_->AddTimer(te);
}

void EventLoop::DelTimer(int timerId)
{
    timerQueue_->DelTimer(timerId);
}