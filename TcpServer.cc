#include "TcpServer.h"
#include "logger.h"

EventLoop* CheckLoopNotNull(EventLoop* loop)
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
    if(started_++ == 0)
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}