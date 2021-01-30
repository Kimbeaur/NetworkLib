/**************************************************
 * @Description 
 * @Author kimbeaur
 * @Date 2021/1/30 09:42
 *************************************************/

#include "BufferPool.h"
#include <assert.h>

//单例的对象
BufferPool* BufferPool::_instance = NULL;

//用于保证创建单例的一个方法全局只执行一次
pthread_once_t BufferPool::_once = PTHREAD_ONCE_INIT;

//初始化锁
pthread_mutex_t BufferPool::_mutex = PTHREAD_MUTEX_INITIALIZER;


void BufferPool::makeIoBufferList(int cap, int num)
{
    //链表的头指针
    IOBuffer *prev;

    //开辟4k buf内存池 
    _pool[cap] = new IOBuffer(cap);
    if (_pool[cap] == NULL) {
        fprintf(stderr, "new IOBuffer %d error", cap);
        exit(1);
    }

    prev = _pool[cap];

    for (int i = 1; i < num; i++) {
        prev->next = new IOBuffer(cap);
        if (prev->next == NULL) {
            fprintf(stderr, "new IOBuffer cap error");
            exit(1);
        }
        prev = prev->next;
    }

    _total_mem += cap/1024 * num;    
}




//构造函数私有化
//BufferPool --> [m4K] ---> (IOBuffer)..IOBuffer..IOBuffer
//             [m16K] --> IOBuffer ..IOBuffer ..IOBuffer
//             ...
BufferPool::BufferPool():_total_mem(0)
{
    makeIoBufferList(m4K, 5000);
    makeIoBufferList(m16K, 1000);
    makeIoBufferList(m64K, 500);
    makeIoBufferList(m256K,200);
    makeIoBufferList(m1M, 50);
    makeIoBufferList(m4M, 20);
    makeIoBufferList(m8M, 10);
}


//从内存池申请一块内存
IOBuffer *BufferPool::AllocBuffer(int N)
{
    int index;
    // 1 找到N最接近的 一个刻度链表 返回一个IOBuffer
    if (N <= m4K) {
        index = m4K;
    }
    else if ( N <= m16K) {
        index = m16K;
    }
    else if ( N <= m64K) {
        index = m64K;
    }
    else if ( N <= m256K) {
        index = m256K;
    }
    else if ( N <= m1M) {
        index = m1M;
    }
    else if ( N <= m4M) {
        index = m4M;
    }
    else if ( N <= m8M) {
        index = m8M;
    }
    else {
        return NULL;
    }


    //2 如果该index内存已经没有了，需要额外的申请内存 
    pthread_mutex_lock(&_mutex);
    if (_pool[index] == NULL) {
        //index链表为空，需要新申请index大小的IOBuffer
        
        if ( _total_mem + index/1024 >= MEM_LIMIT ) {
            fprintf(stderr, "already use too many memory!\n");
            exit(1);
        }

        IOBuffer *new_buf = new IOBuffer(index);
        if (new_buf == NULL) {
            fprintf(stderr, "new IOBuffer error\n");
            exit(1);
        }

        _total_mem += index/1024;
        pthread_mutex_unlock(&_mutex);
        return new_buf;
    }

    //3 如果index有内存， 从pool中拆除一块内存返回
    IOBuffer *target = _pool[index];
    _pool[index] = target->next;
    pthread_mutex_unlock(&_mutex);

    target->next = NULL;

    return target;
}

IOBuffer *BufferPool::AllocBuffer()
{
    return AllocBuffer(m4K);
}

//重置一个IOBuffer 放回pool中
void BufferPool::revert(IOBuffer *buffer)
{
    //将buffer放回pool中
    //index 是属于pool中哪个链表的
    int index = buffer->capacity; 

    buffer->length = 0;
    buffer->head = 0;

    pthread_mutex_lock(&_mutex);

    //断言，一定能够找到index key
    assert(_pool.find(index) != _pool.end());
#if 0
    if (_pool.find(index) == _pool.end()) {
        exit(1);
    }
#endif

    //将buffer设置为对应 buf链表的首节点
    buffer->next = _pool[index];
    _pool[index] = buffer;

    pthread_mutex_unlock(&_mutex);
}
