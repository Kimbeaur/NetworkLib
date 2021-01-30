
/**************************************************
 * @Description 
 * @Author kimbeaur
 * @Date 2021/1/30 09:42
 *************************************************/
#pragma once

#include <ext/hash_map>
#include "IOBuffer.h"

typedef __gnu_cxx::hash_map<int, IOBuffer*> pool_t;

//定义一些内存的刻度
enum MEM_CAP {
    m4K = 4096,
    m16K = 16384,
    m64K = 65536,
    m256K = 262144,
    m1M = 1048576,
    m4M = 4194304,
    m8M = 8388608,
};

//总内存池的大小限制 单位是kb
#define MEM_LIMIT (5U * 1024 * 1024)


class BufferPool {
public:

    //初始化单例对象
    static void init() {
        _instance = new BufferPool();
    }

    //提供一个静态的获取instance的方法
    static BufferPool* instance() {
        //保证 init方法在进程的生命周期只执行一次
        pthread_once(&_once, init);
        return _instance;
    }

    //从内存池申请一块内存
    IOBuffer *AllocBuffer(int N);
    IOBuffer *AllocBuffer();

    //重置一个IOBuffer 放回pool中
    void revert(IOBuffer *buffer);

    void makeIoBufferList(int cap, int num);

private:
    // =========== 创建单例 ==============
    //构造函数私有化
    BufferPool();
    BufferPool(const BufferPool&);
    const BufferPool& operator=(const BufferPool&);
    //单例的对象
    static BufferPool *_instance;
    //用于保证创建单例的一个方法全局只执行一次
    static pthread_once_t _once;

    // ==========  BufferPool 属性======== 
    //存放所有IOBuffer的map句柄    
    pool_t _pool; 

    //当前内存池总体大小 单位是kb
    uint64_t _total_mem;

    //保护pool map增删改查的锁
    static pthread_mutex_t _mutex;
};


