#include "Channel.h"
#include "EventLoop.h"
#include "logger.h"
#include <sstream>
#include <sys/epoll.h>
#include <assert.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      tied_(false),
      eventHandling_(false),
      addedToLoop_(false)
{
}

Channel::~Channel()
{
  assert(!addedToLoop_);
  assert(!eventHandling_);
}

/*
  现在还不知到哪里会调用tie函数
  TcpConnection:: ConnectionEstabish 中会调用此函数，将channnel和指向TcpConnection的shared_ptr绑定
*/
void Channel::tie(const std::shared_ptr<void> &obj)
{
  tied_ = true;
  tie_ = obj;
}

/*
  当Channel中检测的fd的事件有所改变，需要调用Poller的接口epoll_ctl
  EventLoop   =   ChannelList   +    Poller
*/
void Channel::update()
{
  addedToLoop_ = true;
  // add code。。。
  loop_->updateChannel(this);
}

void Channel::remove()
{
  addedToLoop_ = false;
  // add code ...
  loop_->removeChannel(this);
}

void Channel::handleEvent(TimeStamp receivetime)
{
  // 调用过tie函数才会执行下面的步骤
  if (tied_)
  {
    // 升级为强指针，检测指向TcpConnection的shared_ptr是否还存在
    std::shared_ptr<void> guard = tie_.lock();
    if (guard)
    {
      handleEventWithGuard(receivetime);
    }
  }
  else
  {
    handleEventWithGuard(receivetime);
  }
}

// 根据发生的不同事件调用绑定的回调
void Channel::handleEventWithGuard(TimeStamp receiveTime)
{
  LOG_INFO("Channel handleEvent revents:%d",revents_);
  eventHandling_ = true;
  // EPOLLPRI 紧急带外数据
  if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)){
    if(closeCallback_) closeCallback_();
  }
  if(revents_ & EPOLLERR){
    if(errorCallback_) errorCallback_();
  }
  if(revents_ & (EPOLLIN | EPOLLPRI)){
    if(readCallback_) readCallback_(receiveTime);
  }
  if(revents_ & EPOLLOUT){
    if(writeCallback_) writeCallback_();
  }
  eventHandling_ = false;
}