#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include <assert.h>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop,const std::string& nameArg_)
    :baseLoop_(baseLoop),
    name_(nameArg_),
    numThreads_(0),
    started_(false),
    next_(0)
{
    
}

EventLoopThreadPool::~EventLoopThreadPool()
{

}

void EventLoopThreadPool::start(const ThreadInitCallBack& cb)
{
    started_ = true;
    for(int i = 0;i < numThreads_; ++i)
    {
        char buf[name_.size() + 32] = {0};
        snprintf(buf,sizeof buf,"%s%d",name_.c_str(),i);
        EventLoopThread* t = new EventLoopThread(cb,buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop()); // 底层创建一个线程，绑定一个新的EventLoop，并返回该loop的地址
    }

    // 整个服务端只有一个线程，运行着baseloop
    if(numThreads_ == 0 && cb)
    {
        cb(baseLoop_); 
    }
}

// 如果是工作在多线程环境下，baseloop默认以轮询的方式分配channel给EventLoop
EventLoop* EventLoopThreadPool::getNextLoop()
{
    EventLoop* loop = baseLoop_;

    if(!loops_.empty())
    {
        loop = loops_[next_];
        if(++next_ > loops_.size()) next_ = 0;
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    assert(started_);
    if(loops_.empty())
    {
        return std::vector<EventLoop*>(1,baseLoop_);
    }
    else
    {
        return loops_;
    }
}