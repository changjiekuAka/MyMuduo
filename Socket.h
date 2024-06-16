#pragma once
#include "noncopyable.h"

class InetAddress;

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd);
    
    ~Socket();
    
    int fd() const { return sockfd_; }

    void bind(const InetAddress& sockAddr);
    
    void listen();

    int accept(InetAddress *peersock);

    void shutdownWrite();

    void setTcpNoDelay(bool on); 
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepLive(bool on);
private:
    const int sockfd_;
};