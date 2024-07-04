#include "TcpServer.h"
#include "logger.h"
#include "TcpConnection.h"
#include <functional>
#include <string.h>

static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("baseloop is null");
    }
    return loop;
}


TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const std::string& nameArg,
                     Option option)
    : loop_(CheckLoopNotNull(loop)),
      name_(nameArg),
      ipPort_(listenAddr.toIpPort()),
      acceptor_(new Acceptor(loop_,listenAddr,option == kReuseOption)),
      threadPool_(new EventLoopThreadPool(loop_,name_)),
      connectionCallback_(),
      messageCallback_(),
      nextConnId_(1),
      started_(0)
{
    acceptor_->setNewConnectionCallBack(
        std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for(auto& item : connections_)
    {
        TcpConnectionPtr conn(item.second); // 先创建一个临时的副本，防止reset过后不能再使用了，副本出右括号销毁
        item.second.reset();
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed,conn));
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if(started_++ == 0) // 使用原子变量保证TcpServer只能启动一次
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}

// 一个新客户端的连接到来，acceptor会执行这个回调操作
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    // 轮询算法，选择一个subLoop，来管理channel
    EventLoop* ioLoop = threadPool_->getNextLoop();
    // 组织conn的名字
    char buf[64];
    snprintf(buf,sizeof buf,"-%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s]  - new connection [%s] from %s",
            name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());
    
    sockaddr_in local;
    ::bzero(&local,sizeof local);
    socklen_t addrlen = static_cast<socklen_t>(sizeof local);
    if(::getsockname(sockfd,(sockaddr*)&local,&addrlen) < 0)
    {
        LOG_ERROR("sockets::getlocalAddr");
    } 

    InetAddress localAddr(local);
    /*
        创建一个连接(TcpConnection)时，也会创建它对应Channel，Channel在创建的时候，会指定自身
        所属的loop，在这里接收新连接后，选择了一个subLoop，把这个subLoop作为创建TcpConnection的参数
        也就是为subLoop中的添加了一个新的Channel，也可以说是把这个新连接分配到了subLoop中
    */ 

    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                connName,
                                sockfd,
                                localAddr,
                                peerAddr
    ));
    connections_[connName] = conn;
    // 这里是需要用户定义的回调操作
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    
    // TcpConnection 的注销需要TcpServer自行完成，所以自己定义，然后传给TcpConnection
    // conn-shutdown Poller 通知 channel 执行 closeCallback -》conn的handleClose -》 TcpServer的removeConnection 
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection,this,std::placeholders::_1));
    // 注意这里的传参是conn
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInloop,this,conn));
}

void TcpServer::removeConnectionInloop(const TcpConnectionPtr& conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s \n",
            name_.c_str(),conn->name());
    size_t n = connections_.erase(conn->name());
    (void)n;
    assert(n == 1);
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}