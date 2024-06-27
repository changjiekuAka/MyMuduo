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
class TcpConnection : noncopyable,public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                const std::string& nameArg,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }


    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb)
    { highWaterMarkCallback_ = cb; }
    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }

    // Muduo库还有其他的数据格式，这里就用先用string
    void send(const std::string& buf);

    void connectEstablished();
    void connectDestroyed();

    void shutdown();

private:
    enum StateE { kDisconnected , kConnecting , kConnected , kDisconnecting };

    void setState(StateE state) { state_ = state; }

    // 包含调用下面的Callback的逻辑代码，channel发生相应事件后，会调用这些处理函数
    void handleRead(TimeStamp);
    void handleWrite();
    void handleClose();
    void handleError();

    void shutdownInLoop();
    void sendInLoop(const void *data,size_t len);
    
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
    
    // 每个TcpConnection都有发送缓冲区和接收缓冲区
    Buffer inputBuffer_;
    Buffer outputBuffer_;

};