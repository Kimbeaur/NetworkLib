#pragma once
#include <stdint.h>
#include <vector>
#include <ext/hash_map>
#include "EventBase.h"

class TimerQueue
{
public:
    TimerQueue();
    ~TimerQueue();

    int AddTimer(TimerEvent& te);

    void DelTimer(int timer_id);

    int GetTimerfd() const { return timerFd_; }
    int Size() const { return count_; }

    void GetTimerOut(std::vector<TimerEvent>& fired_evs);
private:
    void ResetTimerOut();

    //heap operation
    void HeapAdd(TimerEvent& te);
    void HeapDel(int pos);
    void HeapPop();
    void HeapHold(int pos);

    std::vector<TimerEvent> eventList_;
    typedef std::vector<TimerEvent>::iterator vecIter;

    __gnu_cxx::hash_map<int, int> position_;

    typedef __gnu_cxx::hash_map<int, int>::iterator mapIter;
    
    int count_;
    int nextTimerId_;
    int timerFd_;
    uint64_t pioneer_;//recent timer's millis
};