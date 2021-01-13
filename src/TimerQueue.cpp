#include <unistd.h>
#include <stdio.h>
#include <strings.h>
#include <pthread.h>
#include <sys/timerfd.h>
#include "TimerQueue.h"

TimerQueue::TimerQueue(): count_(0), nextTimerId_(0), pioneer_(-1/*= uint32_t max*/)
{
    //创建定时器fd
    timerFd_ = ::timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
    if(timerFd_ == -1)
    {
        fprintf(stderr,"timerfd_create()");
    }
    
}

TimerQueue::~TimerQueue()
{
    ::close(timerFd_);
}

//添加定时器
int TimerQueue::AddTimer(TimerEvent& te)
{
    te.timerId_ = nextTimerId_++;
    HeapAdd(te);

    if (eventList_[0].ts_ < pioneer_)
    {
        pioneer_ = eventList_[0].ts_;
        ResetTimerOut();
    }
    return te.timerId_;
}

//删除定时器
void TimerQueue::DelTimer(int timerId)
{
    mapIter it = position_.find(timerId);
    if (it == position_.end())
    {
        printf("no such a timerId %d", timerId);
        return ;
    }
    int pos = it->second;
    HeapDel(pos);

    if (count_ == 0)
    {
        pioneer_ = -1;
        ResetTimerOut();
    }
    else if (eventList_[0].ts_ < pioneer_)
    {
        pioneer_ = eventList_[0].ts_;
        ResetTimerOut();
    }
}
//获取所以定时器事件
void TimerQueue::GetTimerOut(std::vector<TimerEvent>& firedEvs)
{
    std::vector<TimerEvent> reuselist;
    while (count_ != 0 && pioneer_ == eventList_[0].ts_)
    {
        TimerEvent te = eventList_[0];
        firedEvs.push_back(te);
        if (te.interval_)
        {
            te.ts_ += te.interval_;
            reuselist.push_back(te);
        }
        HeapPop();
    }
    for (vecIter it = reuselist.begin(); it != reuselist.end(); ++it)
    {
        AddTimer(*it);
    }
    //停止定时器
    if (count_ == 0)
    {
        pioneer_ = -1;
        ResetTimerOut();
    }
    else//当pioneer_ != eventList_[0].ts，更新定时器事件
    {
        pioneer_ = eventList_[0].ts_;
        ResetTimerOut();
    }
}

//重置timerFd_的定时器
void TimerQueue::ResetTimerOut()
{
    struct itimerspec old_ts, new_ts;
    bzero(&new_ts, sizeof(new_ts));

    if (pioneer_ != (uint64_t)(-1))
    {
        //定时器器更新定时器事件，并添加到timerFd_中
        new_ts.it_value.tv_sec = pioneer_ / 1000;
        new_ts.it_value.tv_nsec = (pioneer_ % 1000) * 1000000;
    }
    //当pioneer_ = -1, new_ts = 0 将停止定时器
    ::timerfd_settime(timerFd_, TFD_TIMER_ABSTIME, &new_ts, &old_ts);
}

//添加定时器事件
void TimerQueue::HeapAdd(TimerEvent& te)
{
    eventList_.push_back(te);
    //update position
    position_[te.timerId_] = count_;

    int curr_pos = count_;
    count_++;

    int prt_pos = (curr_pos - 1) / 2;
    while (prt_pos >= 0 && eventList_[curr_pos].ts_ < eventList_[prt_pos].ts_)
    {
        TimerEvent tmp = eventList_[curr_pos];
        eventList_[curr_pos] = eventList_[prt_pos];
        eventList_[prt_pos] = tmp;
        //更新位置
        position_[eventList_[curr_pos].timerId_] = curr_pos;
        position_[tmp.timerId_] = prt_pos;

        curr_pos = prt_pos;
        prt_pos = (curr_pos - 1) / 2;
    }
}
//删除堆
void TimerQueue::HeapDel(int pos)
{
    TimerEvent to_del = eventList_[pos];
    //update position
    position_.erase(to_del.timerId_);

    //rear item
    TimerEvent tmp = eventList_[count_ - 1];
    eventList_[pos] = tmp;
    //update position
    position_[tmp.timerId_] = pos;

    count_--;
    eventList_.pop_back();

    HeapHold(pos);
}
//获取堆顶元素
void TimerQueue::HeapPop()
{
    if (count_ <= 0) return ;
    //update position
    position_.erase(eventList_[0].timerId_);
    if (count_ > 1)
    {
        TimerEvent tmp = eventList_[count_ - 1];
        eventList_[0] = tmp;
        //update position
        position_[tmp.timerId_] = 0;

        eventList_.pop_back();
        count_--;
        HeapHold(0);
    }
    else if (count_ == 1)
    {
        eventList_.clear();
        count_ = 0;
    }
}

//调整堆结构
void TimerQueue::HeapHold(int pos)
{
    int left = 2 * pos + 1, right = 2 * pos + 2;
    int min_pos = pos;
    if (left < count_ && eventList_[min_pos].ts_ > eventList_[left].ts_)
    {
        min_pos = left;
    }
    if (right < count_ && eventList_[min_pos].ts_ > eventList_[right].ts_)
    {
        min_pos = right;
    }
    if (min_pos != pos)
    {
        TimerEvent tmp = eventList_[min_pos];
        eventList_[min_pos] = eventList_[pos];
        eventList_[pos] = tmp;
        //更新位置
        position_[eventList_[min_pos].timerId_] = min_pos;
        position_[tmp.timerId_] = pos;

        HeapHold(min_pos);
    }
}
