#pragma once
#include "Channel.h"
#include <sstream>
#include <sys/epoll.h>


const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI; 
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop,int fd) 
  :loop_(loop),
  fd_(fd),
  events_(0),
  revents_(0),
  index_(-1),
  tied_(false),
  eventHandling_(false),
  addedToLoop_(false)
{}

Channel::~Channel()
{
}

void Channel::tie(const std::shared_ptr<void>& obj)
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

}



