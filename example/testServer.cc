#include <mymuduo/TcpServer.h>
#include <string>
#include <mymuduo/logger.h>

/**
 * 在这个示例中用户自己设定连接连接成功和连接断开需要做的事情，新的连接进来后，发生相应的事情就会调用
 * 由这个连接的Poller管理和监听，事件发生由Channel调用handleEvent
 * 
*/


class echoServer
{
public:
    echoServer(EventLoop* loop,
                const InetAddress &localAddr,
                const std::string name)
        :server_(loop,localAddr,name),
        loop_(loop)
    {
        server_.setConnectionCallback(
            std::bind(&echoServer::onConnection,this,std::placeholders::_1)
        );

        server_.setMessageCallback(
            std::bind(&echoServer::onMessage,this,std::placeholders::_1,
                        std::placeholders::_2,std::placeholders::_3)
        );

        //server_.setThreadNum(3);
    }
    ~echoServer()
    {

    }

    void start()
    {
        server_.start();
    }
private:

    void onConnection(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_INFO("Connection UP is %s",conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN is %s",conn->peerAddress().toIpPort().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr& conn,
                    Buffer* buf,
                    TimeStamp stamp)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown();
    }

    TcpServer server_;
    EventLoop* loop_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    echoServer server(&loop,addr,"EchoServer-01");
    server.start();
    loop.loop();

    return 0;
}