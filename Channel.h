#pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>
#include <memory>


class EventLoop;
/*
*[DEBUG]可能会报错，前置声明TimeStamp，用对象做参数，形参在想要调用构造函数的时候
*会找不到构造函数报错
*/
//class TimeStamp;


/*
    Channel理解为通道，封装fd和它感兴趣的事件events，EPOLLIN,EPOLLOUT,
    绑定事件发生时的回调操作
*/
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;

    Channel(EventLoop* loop,int fd);
    ~Channel();

    void handleEvent(TimeStamp Receivetime);
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    void tie(const std::shared_ptr<void>&);
    
    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }
    
    // 设置fd相应的事件状态
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回fd当前的事件状态
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; } 
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }
    // 一个Channel只能属于一个EventLoop
    EventLoop* ownerLoop() { return loop_; }
    void remove();
private:
    void update();
    void handleEventWithGuard(TimeStamp receiveTime);

    // EPOLLIN EPOLLERR EPOLLOUT EPOLLHUP EPOLLPRI
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;    // 事件循环
    const int  fd_;      // Poller监听fd
    int        events_;  // 注册fd感兴趣的事件
    int        revents_; // Poller返回发生的事件
    int        index_;   // 表示fd处于Poller中的状态

    std::weak_ptr<void> tie_;
    bool tied_;


    /*
        Channel里面可以获知fd中已发生的事件reevents，这里负责调用具体事件的回调操作
    */
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;   

    bool eventHandling_;
    bool addedToLoop_;

};