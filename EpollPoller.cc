#include "EpollPoller.h"
#include "logger.h"
#include <unistd.h>

EpollPoller::EpollPoller(EventLoop* loop)
    : Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)
{
    if(epollfd_ < 0)
    {
        LOG_FATAL("Epollfd create error:%d",errno);        
    }
}

EpollPoller::~EpollPoller()
{
    ::close(epollfd_);
}

