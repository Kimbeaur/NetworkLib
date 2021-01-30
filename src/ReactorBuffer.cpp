
/**************************************************
 * @Description 
 * @Author kimbeaur
 * @Date 2021/1/30 09:42
 *************************************************/

#include "ReactorBuffer.h"
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

ReactorBuf::ReactorBuf()
{
    _buf = NULL;
}

ReactorBuf::~ReactorBuf()
{
    this->clear();
}

//得到当前的buf还有多少有效数据
int ReactorBuf::length()
{
    if (_buf == NULL) {
        return 0;
    }
    else {
        return _buf->length;
    }
}

//已经处理了多少数据
void ReactorBuf::pop(int len)
{
    assert(_buf != NULL && len <= _buf->length);

    _buf->pop(len);

    if (_buf->length == 0) {
        //当前的_buf已经全部用完了
        this->clear(); 
    }
}

//将当前的_buf清空
void ReactorBuf::clear()
{
    if (_buf != NULL) {
        //将_buf 放回BufferPool中
        BufferPool::instance()->revert(_buf);
        _buf = NULL;
    }
}


// -================================================


//从一个fd中读取数据到reactor_buf中
int InputBuf::readData(int fd)
{
    int need_read; //硬件中更有多少数据是可以都的

    //一次性将io中的缓存数据全部都出来
    //需要给fd设置一个属性
    //传出一个参数,目前缓冲中一共有多少数据是可读
    if (ioctl(fd, FIONREAD, &need_read) == -1) {
        fprintf(stderr, "ioctl FIONREAD\n");
        return -1;
    }


    if (_buf == NULL) {
        //如果当前的input_buf里的_buf是空，需要从BufferPool拿一个新的
        _buf = BufferPool::instance()->AllocBuffer(need_read);
        if (_buf == NULL) {
            fprintf(stderr, "no buf for alloc!\n");
            return -1;
        }
    }
    else {
        //如果当前buf可用,判断一下当前buf是否够存
        assert(_buf->head == 0);
        if (_buf->capacity - _buf->length < need_read) {
            //不够存
            IOBuffer *new_buf = BufferPool::instance()->AllocBuffer(need_read+_buf->length);
            if (new_buf == NULL) {
                fprintf(stderr, "no buf for alloc\n");
                return -1;
            }

            //将之前的_buf数据拷贝到新的buf中
            new_buf->copy(_buf);
            //将之前的_buf 放回内存池中
            BufferPool::instance()->revert(_buf);
            //新申请的buf称为当前的IOBuffer
            _buf = new_buf;
        }
    }

    int already_read = 0;

    //当前的buf是可以容纳  读取数据
    do {
        if (need_read == 0) {
            already_read = read(fd, _buf->data + _buf->length, m4K);//阻塞直到有数据
        }
        else {
            already_read = read(fd, _buf->data + _buf->length, need_read);
        }
    } while(already_read == -1 && errno == EINTR);//systemcall一个终端，良性，需要继续读取


    if (already_read > 0) {
        if (need_read != 0) {
            assert(already_read == need_read);
        }

        //读数据已经成功
        _buf->length += already_read;
    }

    return already_read;
}

//获取当前的数据的方法
const char *InputBuf::data()
{
    return _buf != NULL ? _buf->data + _buf->head : NULL;
}

//重置缓冲区
void InputBuf::adjust()
{
    if (_buf != NULL) {
        _buf->adjust();
    }
}

// =========================================== 
    //将一段数据 写到 自身的_buf中
int OutputBuf::sendData(const char *data, int datalen)
{
    if (_buf == NULL) {
        //如果当前的OutputBuf里的_buf是空，需要从BufferPool拿一个新的
        _buf = BufferPool::instance()->AllocBuffer(datalen);
        if (_buf == NULL) {
            fprintf(stderr, "no buf for alloc!\n");
            return -1;
        }
    }
    else {
        //如果当前buf可用,判断一下当前buf是否够存
        assert(_buf->head == 0);

        if (_buf->capacity - _buf->length < datalen) {
            //不够存
            IOBuffer *new_buf = BufferPool::instance()->AllocBuffer(datalen+_buf->length);
            if (new_buf == NULL) {
                fprintf(stderr, "no buf for alloc\n");
                return -1;
            }

            //将之前的_buf数据拷贝到新的buf中
            new_buf->copy(_buf);
            //将之前的_buf 放回内存池中
            BufferPool::instance()->revert(_buf);
            //新申请的buf称为当前的IOBuffer
            _buf = new_buf;
        }
    }

    //将data 数据写到IOBuffer中 拼接到后面    
    memcpy(_buf->data + _buf->length, data, datalen);
    _buf->length += datalen;

    return 0;
}

//将_buf中的数据写到一个fd中
int OutputBuf::write2fd(int fd) //取代 io层 write方法
{
    assert(_buf != NULL && _buf->head == 0);

    int already_write = 0;

    do {
        already_write = write(fd, _buf->data, _buf->length);
    } while (already_write == -1 && errno == EINTR);//系统调用中断产生，不是一个错误


    if (already_write > 0) {
        //已经写成功
        _buf->pop(already_write);
        _buf->adjust();
    }


    //如果fd是非阻塞的,会报already_write==-1 errno==EAGAIN
    if (already_write == -1 && errno == EAGAIN) {
        already_write = 0;//表示非阻塞导致的-1 不是一个错误,表示是正确的只是写0个字节
    }

    return already_write;
}

