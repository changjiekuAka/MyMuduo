#pragma once
#include "noncopyable.h"
#include "CallBack.h"
#include "Acceptor.h"
#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "Buffer.h"
/*
    在平常使用Muduo库时，使用Tcpserver还需要包含以上的头文件，就使用体验上不是很好，但这
    样会暴露更多的接口，这里为了使用方便就直接包含
*/
#include <string>
#include <memory>
#include <unordered_map>
#include <atomic>

class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReuseOption,
        kReuseOption
    };
    
    TcpServer(EventLoop* loop,
              const InetAddress& listenAddr,
              const std::string& nameArg,
              Option option = kNoReuseOption);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    
    // 设置底层subloop的数量
    void setThreadNum(int numThreads);
    // 开启服务器监听 Accptor.listen()  以及启动EventLoopThreadPool
    void start();

private:
    void newConnection(int sockfd,const InetAddress& listenAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInloop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string,TcpConnectionPtr>;

    EventLoop* loop_;  // baseloop用户定义的loop
    const std::string name_;
    const std::string ipPort_;
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_; // One loop per thread
    ConnectionCallback connectionCallback_; //  有新连接到来的回调
    MessageCallback messageCallback_; //  有读写消息的回调
    WriteCompleteCallback writeCompleteCallback_;  //  消息发送完成的回调

    ThreadInitCallback threadInitCallback_; //  loop线程初始化的回调
    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_;
};