#include "TcpServer.h"
#include "logger.h"
#include <functional>

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
      nextConnId_(1)
{
    acceptor_->setNewConnectionCallBack(
        std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));
}

TcpServer::~TcpServer()
{

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

void TcpServer::newConnection(int sockfd, const InetAddress& listenAddr)
{

}