#include "EventLoopThread.h"
#include "EventLoop.h"
#include <iostream>

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,const std::string& name)
    :loop_(nullptr),
    exiting_(false),
    thread_(std::bind(&EventLoopThread::threadFunc,this),name),
    mutex_(),
    cond_(),
    callback_(cb)                            
{

}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if(loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    thread_.start(); // 启动底层线程

    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == nullptr)
        {
            cond_.wait(lock); //  待底层线程创建了一个EventLoop后，就可以继续执行了，也表明此时One loop per thread 形成了
        }
        loop = loop_;
    }
    return loop;
}

// 传递给底层线程的函数，线程启动后执行这个函数
void EventLoopThread::threadFunc()
{
    EventLoop loop; //  创建一个独立的EventLoop，和上面的线程是一一对应的，One loop per thread

    if(callback_) // 执行线程初始化回调操作，这里的参数由EventloopThreadPool在启动的时候传给EventloopThread
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    
    loop.loop(); // EventLoop ==> poll()
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}