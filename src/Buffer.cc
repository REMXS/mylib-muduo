#include<unistd.h>
#include<algorithm>
#include<sys/uio.h>
#include<cstddef>
#include<cerrno>

#include"Buffer.h"


explicit Buffer::Buffer(size_t initial_size=kInitialSize)
    :buffer_(kInitialSize)
    ,read_index_(kCheapPrepend)
    ,write_index_(kCheapPrepend)
{
}

Buffer::~Buffer()
{
}

/*
这个函数使用了两块空间，使用readv读取时先读取到buffer的可读空间，
如果空间不够再使用额外的64kb的栈空间
 */
ssize_t Buffer::readFd(int fd,int& error)
{
    char extra_buffer[65536]={0};

    //使用iovec连续分配两个缓冲区
    iovec vec[2];
    const size_t writeable=writeableBytes();
    
    //第一个指向buffer的可写空间
    vec[0].iov_base=beginWrite();
    vec[0].iov_len=writeable;

    //第二个指向额外的64kb的栈空间
    vec[1].iov_base=extra_buffer;
    vec[1].iov_len=sizeof(extra_buffer);

    /* 
    这里采用的策略是如果可写的空间大于64kb，则认为这个空间足够大，
    采用最简单的读取方式，简化io操作，提高io操作的效率。

    当主缓冲区空间较小时，使用 extra_buffer 作为辅助，用一次系统调用读取更多数据，
    避免了频繁的 read 调用和 buffer_ 的小规模扩容
     */
    const int vec_cnt=(writeable>sizeof(extra_buffer))?1:2;

    ssize_t n=readv(fd,vec,vec_cnt);
    
    if(n<0)
    {
        error=errno;
    }
    else if(n<writeable)
    {
        write_index_+=n;
    }
    else
    {
        write_index_=buffer_.size();
        append(extra_buffer,n-writeable);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd,int& error)
{
    ssize_t n=write(fd,begin()+read_index_,readableBytes());
    if(n<0)
    {
        error=errno;
    }
    return n;
}

void Buffer::retrieve(size_t len)
{
    if(len<readableBytes())
    {
        read_index_+=len;
    }
    else
    {
        retrieveAll();
    }
}

void Buffer::retrieveAll()
{
    read_index_=kCheapPrepend;
    write_index_=kCheapPrepend;
}

std::string Buffer::retrieveAsString(size_t len)
{
    std::string data(peek(),len);
    retrieve(len);
    return data;
}


std::string Buffer::retrieveAllAsString()
{
    return retrieveAsString(readableBytes());
}

void Buffer::append(const char* str,size_t len)
{
    //保证有足够的可写空间
    ensureWriteableBytes(len);

    std::copy(str,str+len,beginWrite());
    write_index_+=len;
}

void Buffer::ensureWriteableBytes(size_t len)
{
    if(writeableBytes()<len)
    {
        makeSpace(len);
    }
}


void Buffer::makeSpace(size_t len)
{
    
    if(prependableBytes()+writeableBytes()<len+kCheapPrepend)
    {
        //如果前置废弃的区域加上可写的区域不够用，则不进行移动，直接进行扩容
        buffer_.resize(write_index_+len);
    }
    else
    {
        size_t read_data_size=readableBytes();
        //移动可读区域
        std::copy(begin()+read_index_,begin()+write_index_,begin()+kCheapPrepend);
        read_index_=kCheapPrepend;
        write_index_=read_index_+read_data_size;
    }
}






