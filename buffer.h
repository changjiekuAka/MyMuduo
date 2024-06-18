#pragma once
#include <string.h>
#include <string>
#include <vector>
#include <assert.h>


/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

/*
    理解Buffer可以这样想，readerIndex和writerIndex控制的滑动窗口
    开始时，readerIndex和writerIndex处的位置都是kCheapPrepend，
    数据写入writerIndex往后移动，writableBytes()表示剩余可写空间
    数据读出readerIndex往后移动，readableBytes()表示剩余可读空间
*/ 

/// 问题：prependable 是数据头，防止粘包问题？ 如何防止的？Buffer在使用时该怎么用
        

class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
    : buffer_(kCheapPrepend + initialSize)
    , readerIndex_(kCheapPrepend)
    , writerIndex_(kCheapPrepend)
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == kInitialSize);
        assert(prependableBytes() == kCheapPrepend);
    } 

    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    // 已经读取的数据长度 + kCheapPrepend
    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    // 置位函数
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        if (len < readableBytes())
        {
            readerIndex_ += len; // 应用只读取了一部分的数据，也就是len，还剩下readerIndex += len  到  writerIndex
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    void retrieveAllAsString()
    {
        retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string reslut(peek(),len);
        retrieve(len);
        return reslut;
    }

    void append(const void* data,size_t len)
    {
        append(static_cast<const char*>(data),len);
    }

    void append(const char* data,size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data,data + len,beginWrite());
        hasWritten(len);
    }

    void ensureWritableBytes(size_t len)
    {
        if(writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    void hasWritten(size_t len)
    {
        writerIndex_ += len;
    }
    size_t readFd(int sockfd,int *saveErrno);
private:
    char* begin() { return &*buffer_.begin(); }
    const char* begin() const { return &*buffer_.begin(); }
    
    void makeSpace(size_t len)
    {
    /*
  		本来缓冲区的结构是这样的
  		|  kCheapPrepend  |    reader    |     writer     |

  		在这里表示的比较是这样的空间比较
  		|  prependableBytes  |  reader   |     writer     |
  		
  		|  kCheapPrepend  |  reader  |        len          |

		prependableBytes表示的是kCheapPrepend + reader余留的空间
		
  		比较式可以翻译成这样：
  		kCheapPrepend + reader余留 + writer可写      <  kCheapPrepend +  len
  		意思也就是说：reader余留的空间，这些空间是可以挪给writer使用的，但是现在writer的空间加上
  		reader余留的也不够要写入的len长度的数据
  		所以选择扩容到writerIndex + len的大小
  	*/
        if(prependableBytes() + writableBytes() < kCheapPrepend + len)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
        /*
		    走到这里表示 reader余留的空间 + writer 足够len长度的数据
		    所以这里可以把reader空间中的数据移动到预留空间的位置，向前移动
		    |  kCheapPrepend  | 余留   | reader |	   writer	  |
								
		    |  kCheapPrepend  | reader | 余留 |      writer	  |
		    这样余留的空间+writer就足够len长度的数据使用了
	    */
        // move readable data to the front, make space inside buffer
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kCheapPrepend
            );
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};