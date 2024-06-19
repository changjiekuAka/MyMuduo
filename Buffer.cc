#include "Buffer.h"
#include <sys/uio.h>

/*
    Poller工作在LT模式下
    从fd中读取流式数据，并不知道数据的大小，但缓冲区大小有限
    readv()函数十分重要，完美地处理这种场景，同样的还有writev()，值得学习
*/

size_t Buffer::readFd(int sockfd,int *saveErrno)
{   
    char extraBuf[65536] = {0};  // 开在栈上的空间 64K同时也是缓冲区的大小，Buffer缓冲区申请的空间在堆上面

    const size_t writable = writableBytes();  // 可写空间的大小
    struct iovec vec[2];
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len =  writable;

    vec[1].iov_base = extraBuf;
    vec[1].iov_len = sizeof extraBuf;

    const int iovcnt = (writable < sizeof extraBuf) ? 2 : 1; // 如果可写空间的大小小于extrabuf，就使用extrabuf
    ssize_t n = ::readv(sockfd,vec,iovcnt);  // readv从fd读取到数据后，可以把数据存放到多处不同内存的区域
    if(n < 0)
    {
        *saveErrno = errno;
    }
    else if( n < writable)  // Buffer可写缓冲区已经够存放读出来的数据了
    {
        writerIndex_ += n;
    }
    else // extrabuf中也写有数据，把writerIndex_调到buffer_末尾，因为此时buffer_已经被写满了
    {
        writerIndex_ = buffer_.size();
        append(extraBuf,n - writable);
    }
    return n;
}