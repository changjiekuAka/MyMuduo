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
    void fillActiveChannnels(int numEvents,ChannelList* activeChannel) const;
    // 更新Channel通道
    void update(int operation,Channel* channel);

    using EventList = std::vector<epoll_event>; 
    
    int epollfd_;
    
    /*
        储存Epoll中关心的事件，调用epoll_wait时，
        需要用一个数组来存储就绪事件，并且会返回就绪事件的个数
        
        epoll_event  [events]
                     [data.ptr]
                     [data.fd]

        events为关心的事件
        其中ptr指向fd的Channel，这样当事件发生时就可以直接去调用Channel中已设置的CallBack
        fd则是epoll监听的套接字
    */ 
    EventList events_;
};
