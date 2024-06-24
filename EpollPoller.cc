#include "EpollPoller.h"
#include "logger.h"
#include "Channel.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>

const int kNew = -1; // 未注册到channels_表，也未注册到Epoll
const int kAdded = 1; // 已注册到channels_表，已注册到Epoll
const int kDeleted = 2; // 已注册到channels_表，已从Epoll中删除


EpollPoller::EpollPoller(EventLoop* loop)
    : Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)
{
    if(epollfd_ < 0)
    {
        LOG_FATAL("Epollfd create error:%d",errno);        
    }
}

EpollPoller::~EpollPoller()
{
    ::close(epollfd_);
}

TimeStamp EpollPoller::Poll(int timeoutMs,ChannelList* activeChannel)
{
    // 这里用DEBUG更合适
    LOG_INFO("FUNC=%s => fd total count:%d\n",channels_.size());
    int numEvents = ::epoll_wait(epollfd_,
                                &*events_.begin(),
                                static_cast<int>(events_.size()),
                                timeoutMs);
    TimeStamp now(TimeStamp::now());
    // 因为有一个EventLoop就会有一个线程，那么当其他线程在Poll时出现事故时，会修改全局的errno
    // 这里提前保存errno的值，防止因为其他线程改变本来的errno
    int saveErrno = errno;
    if(numEvents > 0)
    {
        fillActiveChannnels(numEvents,activeChannel);
        if(numEvents == static_cast<int>(events_.size()))
        {
            events_.resize(events_.size() * 2);
        }
    }else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout \n",__FUNCTION__);
    }else
    {
        if(saveErrno != EINTR){
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() %d",errno);
        }
    }
    return now;
}

void EpollPoller::updateChannel(Channel* channel) 
{
    const int index = channel->index();
    LOG_INFO("FUNC: %s => fd:%d update channel. events=%d index=%d \n",__FUNCTION__,channel->fd(),channel->events(),index);
    if(index == kNew || index == kDeleted)
    {
        int fd = channel->fd();
        if(index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else
        {
            // kDeleted状态，为被删除的状态，从epoll中注销掉了，但是channelMap中还有记录
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);
    }
    else
    {
        int fd = channel->fd();
        // 走到这个步骤，下面的条件都必须满足
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(channel->index() == kAdded);
        
        // 更新channel时，发现channel没有关心的事件，判定为注销该channel
        // channelMap中并没有删除该记录,设置状态为kDeleted
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD,channel);
        }
    }
}

void EpollPoller::removeChannel(Channel* channel)
{
    int fd = channel->fd();
    LOG_INFO("FUNC: %s => fd:%d remove channel. \n",__FUNCTION__,channel->fd());

    // 同样的道理走到这一步，channelMap中理应有该记录
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());

    const int index = channel->index();
    assert(index == kAdded || index == kDeleted);// 已注册，或者已注销但channelMap中有记录

    int n = channels_.erase(fd);
    assert(n == 1);
    
    if(index == kAdded){
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);
}

/*!!!!!*/
void EpollPoller::update(int operation,Channel* channel)
{
    epoll_event event;
    memset(&event,0,sizeof event);
    int fd = channel->fd();
    // 填写epoll_event
    event.events = channel->events();
    event.data.ptr = channel;
    event.data.fd = fd;

    if(::epoll_ctl(epollfd_,operation,fd,&event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("EPOLL_CTL_DEL fd:%d",fd);
        }
        else
        {
            LOG_FATAL("EPOLL_CTL_ADD EPOLL_CTL_MOD fd:%d",fd);
        }
    }
}
// 当epoll_wait返回后，用此函数填写，活跃的Channel(fd)
/*
    解释：调用epoll_ctl，添加events时，往结构体中的ptr参数赋值的是对应Channel的指针
    当有事件发生时，epoll_wait返回，得到发生事件的events数组，得到发生事件的Channel，
    调用Channel中设置的对应回调函数
*/
void EpollPoller::fillActiveChannnels(int numEvents,ChannelList* activeChannel)
{
    for(int i = 0 ; i < numEvents ; i++)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannel->push_back(channel);
    }
}