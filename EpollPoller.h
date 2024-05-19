#pragma once

#include "Poller.h"
#include <sys/epoll.h>

class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller() override;

    TimeStamp Poll(int timeoutMs,ChannelList* activeChnnel) override;
    void updateChannel(Channel* Channel) override;
    void removeChannel(Channel* Channel) override;

private:
    static const int kInitEventListSize = 16;
    // 填写活跃的连接
    void fillActiveChannnels(int numEvents,ChannelList* activeChannel);
    // 更新Channel通道
    void update(int operation,Channel* channel);

    using EventList = std::vector<epoll_event>; 
    
    int epollfd_;
    EventList events_;
};
