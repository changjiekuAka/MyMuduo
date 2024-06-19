#pragma once
#include "noncopyable.h"
#include "InetAddress.h"
#include "CallBack.h"
#include "Buffer.h"
#include <string>
#include <atomic>
#include <memory>

class EventLoop;
class Channel;
class Socket;

// 描述已连接用户的读写事件
class TcpConnection
{
public:
private:
    enum StateE { kDisconnected , kConnecting , kConnected , kDisconnecting };

    EventLoop* loop_; // 肯定绝对不是baseLoop，因为TcpConnection是在subLoop管理的，描述的是一个套接字的连接状态信息
    const std::string name_;
    std::atomic<StateE> state_;
    bool reading;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_;
    Buffer inputBuffer_;
};