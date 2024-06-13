#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
#include "InetAddress.h"
#include <functional>

class EventLoop;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallBack =  std::function<void(int,const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& acceptAddr,bool reusePort);
    ~Acceptor();

    void setNewConnectionCallBack(const NewConnectionCallBack& cb)
    {
        newConnectionCallBack_ = std::move(cb);
    }

    bool listenning() const { return listenning_;}
private:
    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannnl_;
    NewConnectionCallBack newConnectionCallBack_;
    bool listenning_;
};