#pragma once
#include "noncopyable.h"
#include <functional>
#include <memory>
#include <vector>
class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallBack = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop,const std::string& nameArg);
    ~EventLoopThreadPool();

    void start(const ThreadInitCallBack& cb = ThreadInitCallBack());

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const { return started_; }
    const std::string& name() const { return name_; }
private:
    EventLoop* baseLoop_;
    int numThreads_;
    int next_;
    bool started_;
    std::string name_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};