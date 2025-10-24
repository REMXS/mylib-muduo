#pragma once

#include<vector>
#include<string>


//内存结构
/* 
+-------------------+------------------+------------------+
| prependable bytes |  readable bytes  |  writable bytes  |
+-------------------+------------------+------------------+
|                   |                  |                  |
0      <=      readerIndex_     <=  writerIndex_  <=    size() 
可读的区域用于应用层读取数据或者是将数据写入到fd的发送缓冲区，
可写的区域用于应用层写入发送数据或者是读取fd接收缓冲区中的数据
*/
class Buffer
{
private:
    std::vector<char>buffer_;

    size_t read_index_;
    size_t write_index_;

    //返回缓冲区的起始地址
    const char* begin() const{return &*buffer_.begin();}

    char* begin(){return &*buffer_.begin();}

    //确保有len大小的可写空间
    void ensureWriteableBytes(size_t len);

    //将可写的空间大小调整到至少len大小
    void makeSpace(size_t len);
public:
    static const size_t kCheapPrepend = 8;//初始预留的prependabel空间大小
    static const size_t kInitialSize = 1024;


    explicit Buffer(size_t initial_size=kInitialSize);
    ~Buffer();

    ssize_t readFd(int fd,int& error);

    ssize_t writeFd(int fd,int& error);


    //更新可读的缓冲区的状态，就是向前偏移read_index_，表明上层已经处理好上面的数据了
    void retrieve(size_t len);

    void retrieveAll();

    //更新可读的缓冲区的状态,并将偏移过的数据作为字符串返回
    std::string retrieveAsString(size_t len);
    
    std::string retrieveAllAsString();

    size_t readableBytes(){return write_index_-read_index_;}//可读的空间
    size_t writeableBytes(){return buffer_.size()-write_index_;}//可写的空间
    size_t prependableBytes(){return write_index_;}//可读空间前的空间的大小

    //向可写的空间写入字符串
    void append(const char* str,size_t len);

    //返回可读空间的起始地址
    const char* peek()const {return begin()+read_index_;}

    //返回可写空间的起始地址
    char* beginWrite(){return begin()+write_index_;}
    const char* beginWrite()const {return begin()+write_index_;}

};


