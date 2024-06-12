#include "Socket.h"
#include "InetAddress.h"
#include "logger.h"
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

Socket::Socket(int sockfd) 
    :sockfd_(sockfd)
{}

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bind(const InetAddress& addr)
{
    if(0 != ::bind(sockfd_,(sockaddr*)addr.getSockAddr(),sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd:%d fall \n",sockfd_);
    }   
}

void Socket::listen()
{
    if(0 != ::listen(sockfd_,1024))
    {
        LOG_FATAL("listen sockfd:%d fall \n",sockfd_);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    memset(&addr,0,sizeof addr);
    socklen_t len;
    int connfd = ::accept(sockfd_,(sockaddr*)&addr,&len);
    if(connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if(0 != ::shutdown(sockfd_,SHUT_WR))
    {
        LOG_ERROR("shutdownWrite error");
    }
}

