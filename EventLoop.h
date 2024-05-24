#pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include <vector>
#include <functional>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;


// 时间循环类  主要包含了两大模块 Channel   Poller (epoll的抽象)
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    void loop();

    void quit();

    TimeStamp pollReturnTime() const {return pollReturnTime_;}

    // 在当前loop中执行cb
    void runInLoop(Functor cb);
    // 把cd放到队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    void wakeup();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    bool isInLoopThread() const {threadId_ == CurrentThread::tid();}

private:
    void handleRead();
    void doPendingFunctors();


    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;
    std::atomic_bool quit_; // 标志退出loop循环

    const pid_t threadId_; // 记录当前loop所在的线程Id

    TimeStamp pollReturnTime_; // poller返回事件的Channel的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupfd_; // 主要作用唤醒subloop  使用函数eventfd()
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;
    Channel* currentActiveChannel_;

    std::atomic_bool callingPendingFunctors_; // 标志当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; // 存储loop需要执行的所有的回调操作
    std::mutex mutex_; // 用来保护上面的vector
    
};