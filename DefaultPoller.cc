#include "Poller.h"

#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        // return Poll
        return nullptr;
    }
    else
    {
        // return Epoll
        return nullptr;
    }
}
