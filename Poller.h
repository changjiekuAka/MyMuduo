#pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include <vector>
#include <unordered_map>

class EventLoop;
class Channel;

class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;
    
    Poller(EventLoop* loop) : ownerLoop_(loop) {}
    virtual ~Poller();
    
    //相当于epoll_wait，activeChannel表示激活的chnnel，只能在所属EventLoop中调用
    virtual TimeStamp Poll(int timeoutMs,ChannelList* activeChannel) = 0;

protected:
    using ChannelMap = std::unordered_map<int,Channel*>;
    ChannelMap channels;
private:
    EventLoop* ownerLoop_;
};