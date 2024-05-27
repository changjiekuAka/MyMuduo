#include "EventLoop.h"
#include "logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>

// 一个线程只能有一个EventLoop实例
__thread EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;

int createEventfd()
{
    int evtfd = ::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("Failed in eventfd");
    }
    return evtfd;   
}

EventLoop::EventLoop()
    : looping_(false),
    quit_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    wakeupfd_(createEventfd()),
    wakeupChannel_(new Channel(this,wakeupfd_)),
    currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop create %p in thread %d",this,threadId_);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another Eventloop %p exists in this thread %d\n",t_loopInThisThread,threadId_);
    }
    else{
        t_loopInThisThread = this;
    }
    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    /*
        猜测：每个EventLoop都有一个自己的wakeupfd，每个EventLoop有一个Poller，在Poller里面注册wakeupfd
        负责监听wakeupfd的读事件，当一个EventLoop的wakeupfd有数据可读时，表示这个EventLoop需要被唤醒，
        main Reactor 组织了新的Channel需要这个 sub Reactor 处理

        如何实现被唤醒的：在loop函数中会不断地调用Poller::poll函数，这个函数会阻塞地等待是否有事件就绪
        当wakeupfd被注册后，读事件也就被监听，事件发生则Poller::poll函数返回，EventLoop被唤醒
    */
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    // 设置wakeupfd对读事件感兴趣
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupfd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupfd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8",n);
    }
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    while(!quit_)
    {
        activeChannels_.clear();
        /*
            不断获取是否有事件发生
            监听两种fd    一种是client的fd    一种是wakeupfd   正如上面所说的监听wakeupfd的目的
        */
        pollReturnTime_ = poller_->Poll(kPollTimeMs,&activeChannels_);
        
        for(Channel* channel:activeChannels_)
        {
            // handleEvent函数是根据发生的事件类型，来调用事先预设好的对应事件的回调操作
            channel->handleEvent(pollReturnTime_);
        }

        /*
            执行当前EventLoop事件循环需要处理的回调操作
            当调用setThreadNumber函数后，就不会只有一个EventLoop

            mainLoop  事先注册一个回调cb(需要sub loop处理)       
            
            wakeup唤醒后，执行下面的方法，执行mainLoop希望subLoop做的回调操作
        */
        doPendingFunctors();
    }
}