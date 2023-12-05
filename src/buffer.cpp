#include "buffer.h"

Buffer::Buffer(int initBuffersize):buffer_(initBuffersize),readPos_(0),writePos_(0){}

//缓存区中可以读取的字节数
size_t Buffer::readableBytes() const
{
    return writePos_-readPos_;
}

//缓存区中可以写入的字节数
size_t Buffer::writeableBytes() const
{
    return buffer_.size()-writePos_;
}

//缓存区中已经读取的字节数
size_t Buffer::readBytes() const
{
    return readPos_;
}

//获取当前读指针
const char* Buffer::curReadPtr() const
{
    return BeginPtr_()+readPos_;
}

//获取当前写指针
const char* Buffer::curWritePtrConst() const
{
    return BeginPtr_()+writePos_;
}
char* Buffer::curWritePtr()
{
    return BeginPtr_()+writePos_;
}

//更新读数据指针    
void Buffer::updateReadPtr(size_t len)
{
    assert(len<=readableBytes());
    readPos_+=len;
}

void Buffer::updateReadPtrUntilEnd(const char* end)
{
    assert(end>=curReadPtr());
    updateReadPtr(end-curReadPtr());
}

//更新写指针
void Buffer::updateWritePtr(size_t len)
{
    assert(len<=writeableBytes());
    writePos_+=len;
}

//将读指针和写指针初始化
void Buffer::initPtr()
{
    bzero(&buffer_[0],buffer_.size());
    readPos_=0;
    writePos_=0;
}

//用于缓冲区空间不够时的扩容
void Buffer::allocateSpace(size_t len)
{
    // 如果buffer_里面剩余的空间有ln就进行调整，否则需要申请空间。
    // 剩余空间包括write指针之前的空间和可写的空间。
    if(writeableBytes()+readBytes()<len)
    {
        buffer_.resize(writePos_+len+1);
    }
    else{
        //将读指针置为0,调整一下
        size_t readable=readableBytes();
        std::copy(BeginPtr_()+readPos_,BeginPtr_()+writePos_,BeginPtr_());
        readPos_=0;
        writePos_=readable;
        assert(readable==readableBytes());
    }
}

//保证将数据写入缓冲区
void Buffer::ensureWriteable(size_t len)
{
    if(writeableBytes()<len)
    {
        allocateSpace(len);
    }
    assert(writeableBytes()>=len);
}

//将数据写入到缓冲区
void Buffer::append(const char* str,size_t len)
{
    assert(str);
    ensureWriteable(len);
    std::copy(str,str+len,curWritePtr());
    updateWritePtr(len);
}
void Buffer::append(const std::string& str)
{
    append(str.data(),str.length());
}

void Buffer::append(const void* data,size_t len)
{
    assert(data);
    append(static_cast<const char*>(data),len);
}

void Buffer::append(const Buffer& buffer)
{
    append(buffer.curReadPtr(),buffer.readableBytes());
}

//IO操作的读与写接口
ssize_t Buffer::readFd(int fd,int* Errno)
{
    char buff[65535];//暂时的缓冲区
    struct iovec iov[2];
    const size_t writable=writeableBytes();

    iov[0].iov_base=BeginPtr_()+writePos_;
    iov[0].iov_len=writable;
    iov[1].iov_base=buff;
    iov[1].iov_len=sizeof(buff);

    const ssize_t len=readv(fd,iov,2);

    if (len<0)
    {
        //std::cout<<"从fd读取数据失败！"<<std::endl;
        *Errno=errno;
    }
    else if(static_cast<size_t>(len)<=writable)
    {
        writePos_+=len;
    }
    else{
        writePos_=buffer_.size();
        append(buff,len-writable);
    }
    return len;
}

ssize_t Buffer::writeFd(int fd,int* Errno)
{
    size_t readSize=readableBytes();
    ssize_t len=write(fd,curReadPtr(),readSize);
    if(len<0)
    {
        //std::cout<<"往fd写入数据失败！"<<std::endl;
        *Errno=errno;
        return len;
    }
    readPos_+=len;
    return len;
}

//将缓冲区的数据转化为字符串
std::string Buffer::AlltoStr()
{
    std::string str(curReadPtr(),readableBytes());
    initPtr();
    return str;
}

char* Buffer::BeginPtr_()
{
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const
{
    return &*buffer_.begin();
}
