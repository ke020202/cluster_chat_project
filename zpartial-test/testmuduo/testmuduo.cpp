// 使用muduo
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<iostream>
#include<functional>
using namespace std;
using namespace muduo; // 使用muduo命名空间
using namespace muduo::net; // 使用muduo网络库的命名空间
using namespace placeholders; // 使用占位符命名空间，方便绑定回调函数

/* 基于muduo网络库开发服务器程序
1. 组合TcpServer对象
2. 创建EventLoop事件循环对象的指针
3. 明确TcpServer对象的构造函数需要什么参数，输出ChatServer的构造函数
4. 在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5. 设置合适的服务端线程数量，muduo库会自己分配线程处理用户的连接和读写事件
*/
class ChatServer
{ 
public:
    // 构造函数，初始化TcpServer对象和EventLoop指针
    ChatServer(EventLoop *loop, // 事件循环对象的指针
        const InetAddress& listenAddr, //Ip地址和端口号
        const string& nameArg) // 服务器的名字
        :_server(loop,listenAddr,nameArg),_loop(loop)
    {
        // 给服务器注册用户连接的创建和断开回调函数
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1)); // 绑定成员函数作为回调函数，this指针传递给成员函数

        // 给服务器注册用户读写事件的回调函数
        _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3)); // 绑定成员函数作为回调函数，this指针传递给成员函数

        // 设置服务器的线程数量，muduo库会自动分配线程处理用户的连接和读写事件
        _server.setThreadNum(4); // 设置线程数量为4，1个IO线程，3个工作线程

    }
    // 开启事件循环，让服务器开始运行
    void start()
    {
        _server.start(); // 启动服务器，开始监听用户的连接请求
    }
private:
    //专门处理用户的连接创建和断开 epoll listenfd accept
    void onConnection(const TcpConnectionPtr& conn) //TcpConnectionPtr：智能指针，管理一个 TCP 连接
    {
        if(conn->connected()) // 判断连接是否建立成功
            cout<<conn->peerAddress().toIpPort()<<" -> "<<conn->localAddress().toIpPort()<<" state:on "<<endl;
        else
        {
            cout<<conn->peerAddress().toIpPort()<<" -> "<<conn->localAddress().toIpPort()<<" state:off "<<endl;
            conn->shutdown(); // 关闭连接，释放资源
            // _loop->quit(); // 退出事件循环，结束服务器运行
        }
            
    }
    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr& conn, // TCP连接的智能指针
        Buffer *buffer, //缓冲区，存储用户发送的数据
        Timestamp time)
    {
        string buf=buffer->retrieveAllAsString(); // 从缓冲区中读取数据，并转换为字符串
        cout<<"recv data:"<<buf<<" time: "<<time.toString()<<endl; // 输出接收到的数据和时间戳
        conn->send(buf); // 将接收到的数据原封不动地发送回去，实现回显功能        
    }
    TcpServer _server; // 组合TcpServer对象
    EventLoop *_loop; // 创建EventLoop事件循环对象的指针
};
int main()
{
    EventLoop loop; // 创建事件循环对象
    InetAddress addr("127.0.0.1",6000); // 创建IP地址和端口号对象
    ChatServer server(&loop,addr,"ChatServer"); // 创建ChatServer对象，传递事件循环对象的指针、IP地址和端口号、服务器名字
    server.start(); // 启动服务器
    loop.loop(); // 开启事件循环，让服务器开始运行

    return 0;
}/*

g++ testmuduo.cpp -o 111 -lmuduo_net -lmuduo_base -lpthread

-lmuduo_net:链接muduo网络库，提供TCP服务器和客户端的功能
-lmuduo_base:链接muduo基础库，提供日志、线程、时间等功能
-lpthread:链接POSIX线程库，提供多线程支持

*/