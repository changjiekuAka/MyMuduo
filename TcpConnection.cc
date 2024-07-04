#include "TcpConnection.h"
#include "Socket.h"
#include "Channel.h"
#include "logger.h"
#include "EventLoop.h"

static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("baseloop is null");
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop,
                            const std::string& nameArg,
                            int sockfd,
                            const InetAddress& localAddr,
                            const InetAddress& peerAddr)
    :loop_(CheckLoopNotNull(loop)),
    name_(nameArg),
    reading(true),
    state_(kConnecting),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop_,sockfd)), //ioLoop,注意不要忘记一个Channel对应一个EventLoop
    localAddr_(localAddr),                      /* 思考：channel在初始化的时候就会指定好自己所属的EventLoop */
    peerAddr_(peerAddr),                        /*       在后面调用channel的enableReading之类的函数，就会在*/
    highWaterMark_(64*1024*1024)   // 64M       /*       EventLoop创建的Poller中进行注册                  */
{
    // 给sockfd的Channel设置相关的回调，Poller通知Channel，Channel会调用相应的回调(handleEvent)
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite,this));
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose,this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError,this));

    LOG_INFO("TcpConnection::ctor[%s] at %p fd=%d",name_,this,sockfd);
    socket_->setKeepAlive(true); // 保持连接活性
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at %p fd=%d state=%d",name_,this,channel_->fd(),state_.load());
}

// 处理读事件一个socket对象的fd对应一个TcpConnection的fd同时也对应一个Channel
void TcpConnection::handleRead(TimeStamp timeStamp)
{
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(socket_->fd(),&saveErrno);
    if(n > 0)
    {
        if(messageCallback_)
        {
            messageCallback_(shared_from_this(),&inputBuffer_,timeStamp);
        }
    }
    else if( n == 0 )
    {
        handleClose();
    }
    else
    {
        errno = saveErrno;
        LOG_ERROR("TcpConnection handleRead error");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if(channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(),&saveErrno);
        if(n > 0)
        {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if(writeCompleteCallback_)
                {
                    // 这个库中本来的写法，目前来看和直接调用没区别，有待思考
                    /*
                        这里就是调用loop内的函数先唤醒当前loop所在线程，可是loop的线程就是TcpConnection所在的线程
                        因为TcpConnection就是由subloop线程管理的
                    */
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                    //writeCompleteCallback_(shared_from_this());
                }
                /*
                    可能handleWrite在写数据的时候，会被shutdown掉，shutdown会将state_设置为kDisconnecting

                */
                if(state_ == kDisconnecting) // 此处的判断和shutdown函数紧密相连
                {
                    shutdownInLoop(); 
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else // 异常调用了handleWrite，如果channel写完了数据就会取消对写事件的监听，如果此时却因为写事件就绪
    {    // 调用了这个函数就出现异常了   
        LOG_INFO("Connection fd = %d is down, no more writing", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    channel_->disableAll();
    setState(kDisconnected);
    TcpConnectionPtr guardThis(shared_from_this());
    // 这些都是由TcpServer传递conn的
    connectionCallback_(guardThis); // 执行连接关闭时用户想要的回调
    closeCallback_(guardThis); // 执行关闭连接的回调
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    if(::getsockopt(socket_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen) < 0)
    {
        LOG_ERROR("TcpConnection::handleError [%s] - SO_ERROR = %d ",name_,errno);
    }else
    {
        LOG_ERROR("TcpConnection::handleError [%s] - SO_ERROR = %d ",name_,optval);
    }

}

/*
    如果是其他线程调用loop所在线程执行该函数，需要先唤醒loop所在线程，也就是调用runInLoop函数
    其中，如果是本线程则执行传入的函数对象，否则唤醒线程再执行
*/
void TcpConnection::send(const std::string& msg)
{
    if(state_ == kConnected)
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(msg.c_str(),msg.size());
        }
        else
        {
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInLoop,
                this,
                msg.c_str(),
                msg.size()
            ));
        }
    }
}

/*
    发送数据 应用发的快 内核写的慢  就需要把待发送的数据写入缓冲区  而且设置了水位回调
*/

void TcpConnection::sendInLoop(const void *data,size_t len)
{
    ssize_t nwrote = 0; // 已写数据
    size_t remaining = len; // 剩余可写数据
    bool faultError = false;

    if(state_ == kDisconnected)
    {
        LOG_ERROR("disconnected,give up writing");
    }
    /*
        想要直接发送数据，就需要保证Buffer缓冲区中没有数据，否则直接发的数据和缓冲区中发出去数据顺序就不同
        如果Buffer中有数据，就要把需要发送的数据先放到缓冲区里面，再发送
        注意：channel_对写事件的监听和Buffer中的可读数据区是紧密相连的，只要Buffer中没有数据就会取消对写
        事件的监听，有数据就会启动监听
    */
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(),data,len);
        if(nwrote > 0)
        {
            remaining = len - nwrote;
            if(remaining == 0 && highWaterMarkCallback_)
            {
                loop_->runInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
            }
        }
        else  // nwrote < 0
        {
            nwrote = 0;
            if(errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    /*
        如果进入下方的if语句，表示数据并没有被完全写完，或者说缓冲区还有数据，需要把数据append到olddata后面
        再启动Poller对该channel的写事件监听，tcp缓冲区有空间，就会通知事件到达，然后执行channnel的写事件回调
        也就是这里TcpConnection给自己对应的channel设置的会调handleWrite，把缓冲区中的数据发送出去
    
    */
    if(!faultError && remaining > 0)
    {
        size_t oldlen = outputBuffer_.readableBytes();
        if(oldlen + remaining >= highWaterMark_
           && oldlen < highWaterMark_  // oldlen必须小于highWaterMark_，因为Buffer中的数据每次超水位线了就会被处理
           && highWaterMarkCallback_)
        {
            loop_->runInLoop(std::bind(highWaterMarkCallback_,
                                        shared_from_this(),
                                        oldlen + remaining));
            outputBuffer_.append(static_cast<const char*>(data) + nwrote,remaining);
            if(!channel_->isWriting())
            {
                channel_->enableWriting();
            }
        }
    }
    
}
// 调用在TcpServer接收一个新连接的时候，只会调用一次    
void TcpConnection::connectEstablished()
{
    setState(kConnecting);
    // 绑定Channel和TcpConnection
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
} 
// 调用在TcpServer将一个TcpConnection从一个Map中消除掉
void TcpConnection::connectDestroyed()
{
    if(state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::shutdown()
{
    if(state_ == kConnected)
    {
        setState(kDisconnecting); // 这里与handleWrite处理缓冲区数据时有关
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,this));
    }
}

/*
    如果缓冲区中还有数据，就不会进行shutdownWrite操作，也不用担心shutdown不了，调用shutdown函数，
    就会把TcpConnection的state_设置为kDisconnecting，只要缓冲区的数据写完后，
    就会进入if(readableBytes == 0) 的语句中，这时就会检测到state_已经变化，调用shutdownInLoop
*/

void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting()) 
    {
        socket_->shutdownWrite(); //关闭写端,这里会触发Poller监听的EPOLLHUP事件，同时调用注册的回调handleClose
    }
}