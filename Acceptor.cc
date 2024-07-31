#include "Acceptor.h"
#include "logger.h"
#include "InetAddress.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,IPPROTO_TCP);
    if(sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err : %d  \n",__FILE__,__FUNCTION__,__LINE__,errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reusePort)
    :loop_(loop),
    acceptSocket_(createNonblocking()),
    acceptChannnl_(loop,acceptSocket_.fd()),
    listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reusePort);
    acceptSocket_.bind(listenAddr);
    acceptChannnl_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor()
{
    acceptChannnl_.disableAll();
    acceptChannnl_.remove();
}

// 启动套接字监听和对读事件感兴趣 
void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannnl_.enableReading();
}

/*
 *思考：在此处acceptor类负责新连接到来后，触发 返回connfd的回调函数，这里acceptor只知道当连接到来
        需要去接收获得connfd，不知道后续的操作应该是什么，这些应该是由上层也就是TcpServer来定义的
        这样就能做到各个模块之间透明，不知道对方会做什么，只做好自己相关的事情
*/

// 当listenfd监听到有新连接到来(读事件就绪)，调用handleRead ==>   accept   接收新链接
void Acceptor::handleRead()
{   
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0)
    {
        LOG_INFO("1111\n");
        if(newConnectionCallBack_)
        {
            newConnectionCallBack_(connfd,peerAddr);
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
        if(EMFILE == errno)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit : %d !\n",__FILE__,__FUNCTION__,__LINE__,errno);
        }
    }
}