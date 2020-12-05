#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "IOBuffer.h"



//构造，创建一个size大小的buf
IOBuffer::IOBuffer(int size)
{
    capacity = size;
    length = 0;
    head = 0;
    next = NULL;

    data = new char[size];
    assert(data);//如果data==NULL 那么程序直接退出
}

//清空数据
void IOBuffer::clear()
{
    length = head = 0;
}

//处理长度为len的数据，移动head
void IOBuffer::pop(int len) //len表示已经处理的数据的长度
{
    length -= len;
    head += len;
}

//将已经处理的数据清空(内存抹去), 将未处理的数据 移至 buf的首地址, length会减小
void IOBuffer::adjust()
{
    if (head != 0) {
        //length == 0 代表全部的数据已经处理完
        if (length != 0) {
            memmove(data, data+head, length);
        }
        head = 0;
    }
}

//将其他的IOBuffer对象拷贝的自己中
void IOBuffer::copy(const IOBuffer *other)
{
    memcpy(data, other->data + other->head, other->length);
    head = 0;
    length = other->length;
}
