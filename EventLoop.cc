#include "EventLoop.h"
#include "logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <assert.h>

// 一个线程只能有一个EventLoop实例
__thread EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;

int createEventfd()
{
    
    int evtfd = ::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    LOG_INFO("wakeupfd : %d\n",evtfd);
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
    wakeupChannel_(new Channel(this,wakeupfd_))
{
    LOG_INFO("EventLoop create %p in thread %d",this,threadId_);
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

    LOG_INFO("EventLoop %p start looping \n", this);
    while(!quit_)
    {
        activeChannels_.clear();
        /*
            不断获取是否有事件发生
            监听两种fd    一种是client的fd    一种是wakeupfd   正如上面所说的监听wakeupfd的目的
        */

        pollReturnTime_ = poller_->Poll(kPollTimeMs,&activeChannels_);
        LOG_INFO("2222\n");
        for(Channel* channel:activeChannels_)
        {
            // handleEvent函数是根据发生的事件类型(EPOLLIN EPOLLOUT)，来调用事先预设好的对应事件的回调操作
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

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}
// 执行委派的回调函数
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    /* 
        这里使用一个临时变量暂存这些需要执行的回调函数的目的：
            这个函数本该是一个逐个执行存储的回调操作的函数，也就意味着他的逻辑应该是执行一个
        从中删一个，那么如果现在需要执行的回调操作比较多，一直占用锁(对这个存储容器操作的会有多个线程)
        可看下面的queueInLoop函数，锁一直被占用那mainLoop想要委派线程执行函数，那么就会在添加函数的哪里
        阻塞住

        这里使用swap函数，直接将存储容器中所有的函数对象取出来，mainLoop就不会存在阻塞的情况了
    */
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor& func : functors)
    {
        func();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::quit()
{
    quit_ = true;

    /*
        调用该方法的线程有两种
        该EventLoop的本来的线程        不属于该EventLoop的其他线程

        比如在一个subloop中(worker)调用了mainLoop(IO)的quit
        会让mainLoop从poll的阻塞等待中跳出，最后循环到检查quit_处结束循环   
    */
    if(!isInLoopThread())
    {
        wakeup();
    }
}

// 往wakeupfd中写入数据，wakeupfd的读事件就绪
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupfd_,&one,sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %d bytes instead of 8",n);
    }
}

// channel会调用这些函数，让EventLoop去操作poller
void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    return poller_->hasChannel(channel);
}

void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread()) // 在当前的loop线程中，执行cb
    {
        cb(); 
    }
    else // 不在当前线程，说明一个线程A调用线程B的runInLoop，线程B需要被唤醒，为什么线程A会调用到？待解决
    {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {   // 可能会有多个线程，会对pendingFunctors_操作
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    /*
        这里的是有两种情况需要唤醒loop所属的线程

        一：调用该函数的线程A不是loop所属的线程B，表示：非loop线程A唤醒线程B去执行回调操作
        二：线程B正在执行回调函数，线程B或者线程A需要唤醒线程B，让线程B在执行完回调操作后，调用poll函数
            不会阻塞，而是去执行新增的回调操作
    */
    if(!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}
