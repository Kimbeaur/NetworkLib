#pragma  once

/*
 * 定义一个 buffer一块内存的数据结构体
 * */

class IOBuffer {
public:
    //构造，创建一个size大小的buf
    IOBuffer(int size);

    //清空数据
    void clear();

    //处理长度为len的数据，移动head
    void pop(int len); //len表示已经处理的数据的长度

    //将已经处理的数据清空(内存抹去), 将未处理的数据 移至 buf的首地址, length会减小
    void adjust();

    //将其他的IOBuffer对象拷贝的自己中
    void copy(const IOBuffer *other);

    //当前buf的总容量
    int capacity;

    //当前buf的有效数据长度
    int length;

    //当前未处理有效数据的头部索引
    int head;

    //当前buf的内存首地址
    char *data;

    //存在多个IOBuffer 采用链表的形式进行管理
    IOBuffer *next;
};

//取数据  IOBuffer.data+head,  lenght 
//IOBuffer.data, 3.
//IOBuffer.pop(3)
//IOBuffer.adjust();

